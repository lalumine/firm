#pragma once

#include <assert.h>
#include "log/log.h"
#include <vector>
#include "lib/scarray.hpp"
#include "fspi_base.hpp"
#include "simple_walk.hpp"

// randon-walk indexing scheme for forasp
class windex_realtime : public simple_walk, public fspi_base {
public:
  template <typename C>
  windex_realtime(graph* g, bool is_dird, C config) :
    fspi_base(g, is_dird, config) { }

  template <typename Vec>
  void adapt(const Vec&, double) { }

  node_id get(node_id s, record_sno) const {
    return random_walk(_g, s, alpha);
  }

  void update_insert(node_id, node_id, edge_sno) { }

  void update_delete(node_id, node_id, edge_sno) { }
};
