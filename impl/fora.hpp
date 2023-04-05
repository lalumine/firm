#pragma once

#include <assert.h>
#include "log/log.h"
#include <algorithm>
#include <vector>
#include "time/timer.hpp"
#include "fora_interface.hpp"
#include "graph.hpp"
#include "uniqueue.hpp"

template <typename H>
class fora :
  public fora_interface,
  public fora_impl_full, public fora_impl_topk
{
private:
  const bool _is_dird;
  graph* const _g;
  H* const _h;

public:
  template <typename C>
  fora(bool is_dird, node_id n, const edge_list& edges, C config) :
    _is_dird(is_dird), _g(new graph(n)), _h(nullptr)
  {
    for (auto [u, v] : edges) {
      _g->insert_edge(u, v);
      if (!is_dird && u != v) _g->insert_edge(v, u);
    }
    *const_cast<H**>(&_h) = new H(_g, is_dird, config);
  }

  econfigs experiment_configs() {
    return econfigs { _h->alpha, _h->eps, _h->det, _h->pf };
  }


  void evaluate_full(node_id s, fora_impl_full::outputer output) {
    fora_impl_full::evaluate(_g, _h, s, output);
  }

  void evaluate_topk(node_id s, node_id k, fora_impl_topk::outputer output) {
    fora_impl_topk::evaluate(_g, _h, s, k, output);
  }

  void insert_edge(node_id u, node_id v) {
    Timer tmr(TIMER::UPDATE);
    std::optional<edge_sno> esno = _g->insert_edge(u, v);
    if (!esno) return;
    _h->update_insert(u, v, esno.value());
    if (!_is_dird && u != v) {
      esno = _g->insert_edge(v, u);
      if (!esno) {
        log_fatal("fail to insert duel edge <%zu, %zu>", (size_t)u, (size_t)v);
        exit(1);
      }
      _h->update_insert(v, u, esno.value());
    }
  }

  void delete_edge(node_id u, node_id v) {
    Timer tmr(TIMER::UPDATE);
    std::optional<edge_sno> esno = _g->delete_edge(u, v);
    if (!esno) return;
    _h->update_delete(u, v, esno.value());
    if (!_is_dird && u != v) {
      esno = _g->delete_edge(v, u);
      if (!esno) {
        log_fatal("fail to delete duel edge <%zu, %zu>", (size_t)u, (size_t)v);
        exit(1);
      }
      _h->update_delete(v, u, esno.value());
    }
  }
};
