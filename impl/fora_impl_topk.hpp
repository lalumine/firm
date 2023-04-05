#pragma once

#include <assert.h>
#include "log/log.h"
#include <algorithm>
#include "time/timer.hpp"
#include "graph.hpp"
#include "sparse_vector.hpp"
#include "uniqueue.hpp"

class fora_impl_topk {
private:
  using ppr_vec = sparse_vector;

  template <typename H>
  void _forward_push(graph* _g, H* _h,
    uniqueue& frontier, ppr_vec& rsv, ppr_vec& rsd, double det)
  {
    static uniqueue tfrontier(_g->num_nodes() + 1);
    static uniqueue queue(_g->num_nodes() + 1);
    Timer tmr(TIMER::PUSH);

    double rmax = _h->rmax(det), rmax0 = _h->rmax(_h->det);
    while (!frontier.empty()) {
      node_id u = frontier.pop();
      // dangling node cannot be in frontier
      if (rsd[u] >= rmax * _g->get_degree(u))
        queue.push(u);
      else if (rsd[u] >= rmax0 * _g->get_degree(u))
        tfrontier.push(u);
    }

    while (!queue.empty()) {
      node_id u = queue.pop();
      rsv.accumulate(u, _h->alpha * rsd[u]);
      // dangling node cannot be in queue
      double detr = (1 - _h->alpha) * rsd[u] / _g->get_degree(u);
      log_trace("on node %zu, residue = %e, increment = %e",
        (size_t)u, rsd[u], detr);
      rsd.update(u, 0);

      for (node_id v : _g->get_neighbourhood(u)) {
        // push method will not be invoked at dangling node
        if (_g->is_dangling_node(v)) rsv.accumulate(v, detr);
        else {
          rsd.accumulate(v, detr);
          if (rsd[v] >= rmax * _g->get_degree(v))
            queue.push(v);
          else if (rsd[v] >= rmax0 * _g->get_degree(v))
            frontier.push(v);
        }
      }
    }
    while (!tfrontier.empty()) frontier.push(tfrontier.pop());

    rsv.iterize();
    rsd.iterize();
  }

  template <typename H>
  void _adapt(H* _h, const ppr_vec& rsd, double det) {
    Timer tmr(TIMER::ADAPT);
    _h->adapt(rsd, det);
  }

  template <typename H>
  void _combine(graph* _g, H* _h,
    ppr_vec& ppr, ppr_vec& rsv, ppr_vec& rsd, double det)
  {
    Timer tmr(TIMER::REFINE);
    log_debug("evaluating...");
    ppr = rsv;
    for (node_id v : rsd) {
      if (_g->is_dangling_node(v)) ppr.accumulate(v, rsd[v]);
      else {
        ppr.accumulate(v, _h->alpha * rsd[v]);
        record_sno c = _h->num_samples(v, rsd[v], det);
        double wgh = (1 - _h->alpha) * rsd[v] / c;
        for (record_sno i = 0; i < c; ++i)
          ppr.accumulate(_h->get(v, i), wgh);
      }
    }
    ppr.iterize();
  }

  bool _check_topk(ppr_vec& ppr, double lim, node_id k) {
    Timer tmr(TIMER::CHECK_K);
    if (ppr.size() < k) return false;
    node_id c = 0;
    for (node_id v : ppr) {
      if (ppr[v] >= lim)
        if (++c == k) return true;
    }
    return false;
  }

  template <typename H>
  void _evaluate(graph* _g, H* _h,
    node_id s, node_id k, ppr_vec& ppr, ppr_vec& rsv, ppr_vec& rsd)
  {
    static uniqueue frontier(_g->num_nodes() + 1);
    Timer tmr(TIMER::EVALUATE);

    size_t num_iter = 0;
    log_debug("push round %zu, det = %e", num_iter++, 1.);
    if (_g->is_dangling_node(s)) {
      ppr.clear();
      ppr.update(s, 1.0);
      return;
    }
    rsd.update(s, 1.0);
    frontier.push(s);

    double dfac = 1. / (log1p(_g->num_nodes()) + log1p(_g->num_edges()) + 1);
    double det = std::max(_h->det, dfac / k);
    while (det >= _h->det) {
      log_debug("push round %zu, det = %e", num_iter++, det);
      _forward_push(_g, _h, frontier, rsv, rsd, det);
      log_debug("preparing...");
      _adapt(_h, rsd, det);
      log_debug("combining...");
      _combine(_g, _h, ppr, rsv, rsd, det);
      log_debug("checking top-k...");
      if (_check_topk(ppr, (1 + _h->eps) * det, k)) break;
      if (det == _h->det) break;
      det = std::max(_h->det, 0x1p-2 * det);
    }

    while (!frontier.empty()) frontier.pop();
  }

public:
  using outputer = std::function<void(const std::vector<node_id>&)>;

private:
  void _output(outputer output, node_id k, ppr_vec& ppr) {
    static std::vector<std::pair<node_id, double>> vppr;
    static std::vector<node_id> topk;
    Timer tmr(TIMER::OUTPUT);

    vppr.clear();
    for (node_id v : ppr) {
      vppr.push_back({v, ppr[v]});
    }
    std::sort(vppr.begin(), vppr.end(), [](const auto& a, const auto& b) {
      return a.second > b.second;
    });
    topk.clear();
    for (size_t i = 0; i < k && i < vppr.size(); ++i) {
      topk.push_back(vppr[i].first);
    }
    output(topk);
  }

protected:
  template <typename H>
  void evaluate(graph* _g, H* _h, node_id s, node_id k, outputer output) {
    static ppr_vec rsv(_g->num_nodes() + 1);
    static ppr_vec rsd(_g->num_nodes() + 1);
    static ppr_vec ppr(_g->num_nodes() + 1);
    rsv.clear();
    rsd.clear();

    _evaluate(_g, _h, s, k, ppr, rsv, rsd);
    _output(output, k, ppr);
  }
};
