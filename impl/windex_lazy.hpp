#pragma once

#include <assert.h>
#include "log/log.h"
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <vector>
#include "lib/scarray.hpp"
#include "fspi_base.hpp"
#include "simple_walk.hpp"
#include "sparse_vector.hpp"
#include "uniqueue.hpp"

template <bool strict>
class windex_lazy : public simple_walk, public fspi_base {
private:
  const double _theta, _epsi;

  std::vector<scarray<node_id>> _redge_list;
  std::vector<std::unordered_map<node_id, edge_sno>> _redge_table;

  double _ssum;
  std::vector<double> _sigma;
  std::vector<std::vector<node_id>> _tpoints;

private:
  void _update_inaccuracy(node_id t, unsigned del) {
    static std::vector<double> rbak(_g->num_nodes() + 1);
    static uniqueue queue(_g->num_nodes() + 1);

    log_debug("updating inaccuracy");
    edge_sno doutt = _g->get_degree(t) + del;
    double rbmax = _is_dird ?
      1. / _g->num_nodes() :
      2. * doutt / (_g->num_edges() + 2 * del);
    std::fill(rbak.begin(), rbak.end(), 0);
    rbak[t] = 1.;
    if (rbak[t] > rbmax) queue.push(t);
    while (!queue.empty()) {
      node_id u = queue.pop();
      double rbaku = rbak[u];
      rbak[u] = .0;
      _sigma[u] += rbaku / doutt;
      for (node_id v : _redge_list[u]) {
        rbak[v] += (1 - alpha) * rbaku / _g->get_degree(v);
        if (rbak[v] > rbmax) queue.push(v);
      }
    }

    _ssum = 0;
    for (node_id v = 1; v <= _g->num_nodes(); ++v) {
      _sigma[v] += rbmax / (alpha * doutt);
      _ssum += _sigma[v];
    }
  }

  double _eval_inaccuracy(
    const std::vector<double>& rsd,
    std::vector<double>& inacc,
    std::priority_queue<std::pair<double, node_id>>& heap)
  {
    double esum = .0;
    for (node_id v = 1, n = _g->num_nodes(); v <= n; ++v) {
      if (rsd[v] == 0 || _sigma[v] == 0 || _tpoints[v].empty()) continue;
      inacc[v] = rsd[v] * _sigma[v];
      esum += inacc[v];
      heap.push(std::make_pair(inacc[v] / _tpoints[v].size(), v));
    }
    return esum;
  }

  double _eval_inaccuracy(
    const sparse_vector& rsd,
    std::vector<double>& inacc,
    std::priority_queue<std::pair<double, node_id>>& heap)
  {
    double esum = .0;
    for (node_id v : rsd) {
      if (_sigma[v] == 0 || _tpoints[v].empty()) continue;
      inacc[v] = rsd[v] * _sigma[v];
      esum += inacc[v];
      heap.push(std::make_pair(inacc[v] / _tpoints[v].size(), v));
    }
    return esum;
  }

  void _insert_redge(node_id u, node_id v) {
    assert(_redge_table[v].find(u) == _redge_table[v].end());
    _redge_list[v].emplace(u);
    _redge_table[v][u] = _redge_list[v].size() - 1;
  }

  void _delete_redge(node_id u, node_id v) {
    assert(_redge_table[v].find(u) != _redge_table[v].end());
    edge_sno resno = _redge_table[v][u];
    _redge_table[v].erase(u);
    _redge_list[v].remove(resno,
      [this, resno, v](node_id uu) { _redge_table[v][uu] = resno; }
    );
  }

  template <typename C>
  C _reconfig(C config) {
    C reconfig = config;
    if constexpr (strict) {
      reconfig.eps = config.theta * config.eps;
    }
    return reconfig;
  }

public:
  template <typename C>
  windex_lazy(graph* g, bool is_dird, C config) :
    fspi_base(g, is_dird, _reconfig(config)),
    _theta(config.theta),
    _epsi((1 - config.theta) * config.eps),
    _redge_list(g->num_nodes() + 1),
    _redge_table(g->num_nodes() + 1),
    _ssum(0), _sigma(g->num_nodes() + 1),
    _tpoints(g->num_nodes() + 1)
  {
    for (node_id u = 1; u <= _g->num_nodes(); ++u) {
      for (node_id v : _g->get_neighbourhood(u))
        _insert_redge(u, v);
    }
    for (node_id v = 1; v <= _g->num_nodes(); ++v)
      for (record_sno wnum = index_size(v); wnum; --wnum)
        _tpoints[v].push_back(random_walk(_g, v, alpha));
  }

  template <typename Vec>
  void adapt(const Vec& rsd, double delta) {
    static std::vector<double> inacc(_g->num_nodes() + 1);

    double emax = _epsi * delta;
    log_debug("updating inaccurate random walks, emax = %e", emax);
    if (_ssum <= emax) return;

    std::fill(inacc.begin(), inacc.end(), 0);
    std::priority_queue<std::pair<double, node_id>> heap;
    double esum = _eval_inaccuracy(rsd, inacc, heap);

    while (esum > emax) {
      node_id v = heap.top().second;
      log_trace("regenerating random-walks starting from %zu", (size_t)v);
      for (record_sno k = 0; k < _tpoints[v].size(); ++k)
        _tpoints[v][k] = random_walk(_g, v, alpha);
      _ssum -= _sigma[v];
      _sigma[v] = 0;
      esum -= inacc[v];
    }
  }

  node_id get(node_id s, record_sno wsno) const {
    return _tpoints[s][wsno];
  }

  void update_insert(node_id u, node_id v, edge_sno) {
    _insert_redge(u, v);
    _update_inaccuracy(u, 0);
    while (index_size(u) > _tpoints[u].size()) {
      log_trace("add new random-walk at node %zu", (size_t)u);
      _tpoints[u].push_back(random_walk(_g, u, alpha));
    }
  }

  void update_delete(node_id u, node_id v, edge_sno) {
    _delete_redge(u, v);
    _update_inaccuracy(u, 1);
    while (index_size(u) < _tpoints[u].size()) {
      log_trace("remove random-walk at node %zu", (size_t)u);
      _tpoints[u].pop_back();
    }
  }
};
