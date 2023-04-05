#pragma once

#include <assert.h>
#include "log/log.h"
#include <vector>
#include "lib/scarray.hpp"
#include "fspi_base.hpp"
#include "simple_walk.hpp"

class windex_eager : public simple_walk, public fspi_base {
private:
  std::vector<path_id> _woffset;
  std::vector<node_id> _tpoints;

  void _reconstruct() {
    _tpoints.clear();
    for (node_id v = 1; v <= _g->num_nodes(); ++v) {
      _woffset[v] = _tpoints.size();
      for (record_sno wnum = index_size(v); wnum; --wnum)
        _tpoints.push_back(random_walk(_g, v, alpha));
    }
  }

public:
  template <typename C>
  windex_eager(graph* g, bool is_dird, C config) :
    fspi_base(g, is_dird, config),
    _woffset(g->num_nodes() + 1),
    _tpoints()
  {
    _reconstruct();
  }

  template <typename Vec>
  void adapt(const Vec&, double) { }

  node_id get(node_id s, record_sno wsno) const {
    return _tpoints[_woffset[s] + wsno];
  }

  void update_insert(node_id u, node_id v, edge_sno) {
    if (!_is_dird && !_g->get_edge_sno(v, u)) return;
    log_debug("reconstructing random walk(s)");
    _reconstruct();
    log_debug("reconstructed %zu random walk(s)", _tpoints.size());
  }

  void update_delete(node_id u, node_id v, edge_sno) {
    if (!_is_dird && _g->get_edge_sno(v, u)) return;
    log_debug("reconstructing random walk(s)");
    _reconstruct();
    log_debug("reconstructed %zu random walk(s)", _tpoints.size());
  }
};
