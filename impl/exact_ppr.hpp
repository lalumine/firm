#pragma once

#include <assert.h>
#include "log/log.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include "time/timer.hpp"
#include "fora_interface.hpp"
#include "graph.hpp"
#include "uniqueue.hpp"

class exact_ppr : public fora_interface {
private:
  const size_t _round;
  const double _alpha;
  const bool _is_dird;

  graph* const _g;
  std::vector<double> __rsv;
  std::vector<double> __rsd;

  void _clear() {
    std::fill(__rsv.begin(), __rsv.end(), 0);
    std::fill(__rsd.begin(), __rsd.end(), 0);
  }

  void _evaluate(node_id s) {
    static uniqueue *q = new uniqueue(_g->num_nodes() + 1);
    static uniqueue *q_next = new uniqueue(_g->num_nodes() + 1);
    Timer tmr(TIMER::EVALUATE);
    Timer tmr2(TIMER::PUSH);

    __rsd[s] = 1.0;
    q->push(s);
    for (size_t i = 0; i < _round; ++i) {
      while (!q->empty()) {
        node_id u = q->pop();
        if (_g->is_dangling_node(u)) {
          __rsv[u] += __rsd[u];
          __rsd[u] = 0;
        } else {
          __rsv[u] += _alpha * __rsd[u];
          double detr = (1 - _alpha) * __rsd[u] / _g->get_degree(u);
          __rsd[u] = 0;
          for (node_id v : _g->get_neighbourhood(u)) {
            __rsd[v] += detr;
            q_next->push(v);
          }
        }
      }
      std::swap(q, q_next);
    }
    assert(q_next->empty());
    while (!q->empty()) q->pop();
  }

  void _output_full(fora_impl_full::outputer output) {
    Timer tmr(TIMER::OUTPUT);
    output(__rsv);
  }

  void _output_topk(fora_impl_topk::outputer output, node_id k) {
    static std::vector<node_id> topk;
    Timer tmr(TIMER::OUTPUT);

    topk.clear();
    for (node_id v = 1; v <= _g->num_nodes(); ++v) {
      if (__rsv[v] > 0) topk.push_back(v);
    }
    std::sort(topk.begin(), topk.end(), [this](node_id u, node_id v) {
      return __rsv[u] > __rsv[v];
    });
    if (topk.size() > k) topk.resize(k);
    output(topk);
  }

public:
  template <typename C>
  exact_ppr(bool is_dird, node_id n, const edge_list& edges, C config) :
    _round(config.round), _alpha(config.alpha),
    _is_dird(is_dird), _g(new graph(n)),
    __rsv(n + 1), __rsd(n + 1)
  {
    for (auto [u, v] : edges) {
      _g->insert_edge(u, v);
      if (!is_dird && u != v) _g->insert_edge(v, u);
    }
  }

  econfigs experiment_configs() {
    return econfigs { _alpha, pow(1 - _alpha, _round), 0, 0 };
  }

  void evaluate_full(node_id s, fora_impl_full::outputer output) {
    _clear();
    _evaluate(s);
    _output_full(output);
  }

  void evaluate_topk(node_id s, node_id k, fora_impl_topk::outputer output) {
    _clear();
    _evaluate(s);
    _output_topk(output, k);
  }

  void insert_edge(node_id u, node_id v) {
    Timer tmr(TIMER::UPDATE);
    std::optional<edge_sno> esno = _g->insert_edge(u, v);
    if (!esno) return;
    if (!_is_dird && u != v) {
      esno = _g->insert_edge(v, u);
      if (!esno) {
        log_fatal("fail to insert duel edge <%zu, %zu>", (size_t)u, (size_t)v);
        exit(1);
      }
    }
  }

  void delete_edge(node_id u, node_id v) {
    Timer tmr(TIMER::UPDATE);
    std::optional<edge_sno> esno = _g->delete_edge(u, v);
    if (!esno) return;
    if (!_is_dird && u != v) {
      esno = _g->delete_edge(v, u);
      if (!esno) {
        log_fatal("fail to delete duel edge <%zu, %zu>", (size_t)u, (size_t)v);
        exit(1);
      }
    }
  }
};
