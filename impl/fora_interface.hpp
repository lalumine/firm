#pragma once

#include <functional>
#include <string>
#include <vector>
#include "fora_impl_full.hpp"
#include "fora_impl_topk.hpp"
#include "graph_types.hpp"

class fora_interface {
public:
  struct econfigs { double alpha, epsilon, delta, pf; };
public:
  virtual void evaluate_full(node_id, fora_impl_full::outputer) = 0;
  virtual void evaluate_topk(node_id, node_id, fora_impl_topk::outputer) = 0;
  virtual void insert_edge(node_id u, node_id v) = 0;
  virtual void delete_edge(node_id u, node_id v) = 0;
  virtual econfigs experiment_configs() = 0;
};
