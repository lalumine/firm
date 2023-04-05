#pragma once

#include <assert.h>
#include "log/log.h"
#include <algorithm>
#include <vector>
#include "time/timer.hpp"
#include "graph.hpp"
#include "uniqueue.hpp"

class fora_impl_full {
private:
  using ppr_vec = std::vector<double>;

  template <typename H>
  void _forward_push(graph* _g, H* _h, ppr_vec& rsv, ppr_vec& rsd, node_id s)
  {
    static uniqueue queue(_g->num_nodes() + 1);
    Timer tmr(TIMER::PUSH);

    double rmax = _h->rmax(_h->det);
    if (_g->is_dangling_node(s)) rsv[s] = 1.0;
    else {
      rsd[s] = 1.0;
      if (rsd[s] >= rmax * _g->get_degree(s)) queue.push(s);
    }
    while (!queue.empty()) {
      node_id u = queue.pop();
      rsv[u] += _h->alpha * rsd[u];
      // dangling node cannot be in queue
      double detr = (1 - _h->alpha) * rsd[u] / _g->get_degree(u);
      log_trace("on node %zu, rsd = %e, inc = %e", (size_t)u, rsd[u], detr);
      rsd[u] = 0;

      for (node_id v : _g->get_neighbourhood(u)) {
        // push method will not be invoked at dangling node
        if (_g->is_dangling_node(v)) rsv[v] += detr;
        else {
          rsd[v] += detr;
          if (rsd[v] >= rmax * _g->get_degree(v)) queue.push(v);
        }
      }
    }
  }

  template <typename H>
  void _adapt(H* _h, const ppr_vec& rsd, double det) {
    Timer tmr(TIMER::ADAPT);
    _h->adapt(rsd, det);
  }

  template <typename H>
  void _combine(graph* _g, H* _h, ppr_vec& rsv, ppr_vec& rsd, double det)
  {
    Timer tmr(TIMER::REFINE);
    for (node_id v = 1, n = _g->num_nodes(); v <= n; ++v) {
      if (_g->is_dangling_node(v)) rsv[v] += rsd[v];
      else {
        rsv[v] += _h->alpha * rsd[v];
        record_sno c = _h->num_samples(v, rsd[v], det);
        double wgh = (1 - _h->alpha) * rsd[v] / c;
        for (record_sno i = 0; i < c; ++i)
          rsv[_h->get(v, i)] += wgh;
      }
    }
  }

  template <typename H>
  void _evaluate(graph* _g, H* _h,
    node_id s, ppr_vec& rsv, ppr_vec& rsd)
  {
    Timer tmr(TIMER::EVALUATE);
    log_debug("forward pushing");
    _forward_push(_g, _h, rsv, rsd, s);
    log_debug("adjusting indecies");
    _adapt(_h, rsd, _h->det);
    log_debug("refining estimation");
    _combine(_g, _h, rsv, rsd, _h->det);
  }

public:
  using outputer = std::function<void(const std::vector<double>&)>;

private:
  void _output(outputer output, const ppr_vec& ppr) {
    Timer tmr(TIMER::OUTPUT);
    output(ppr);
  }

protected:
  template <typename H>
  void evaluate(graph* _g, H* _h, node_id s, outputer output) {
    static ppr_vec rsv(_g->num_nodes() + 1);
    static ppr_vec rsd(_g->num_nodes() + 1);
    std::fill(rsv.begin(), rsv.end(), 0);
    std::fill(rsd.begin(), rsd.end(), 0);

    _evaluate(_g, _h, s, rsv, rsd);
    _output(output, rsv);
  }
};
