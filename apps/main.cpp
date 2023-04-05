#include "log/log.h"
#include <cstdio>
#include <cstring>
#include "apps/types.hpp"
#include "io/file.hpp"
#include "exact_ppr.hpp"
#include "fora.hpp"
#include "windex_eager.hpp"
#include "windex_inc.hpp"
#include "windex_lazy.hpp"
#include "windex_realtime.hpp"

constexpr char help[] =
  "exam <algo_name> <data_path> [options]\n"
  "algo_name: firm, fora, fora+, agenda, agenda*, exact\n"
  "options:\n"
  "  --alpha <alpha>\n"
  "  --epsilon <epsilon>\n"
  "  --delta_exp <exponent of delta>\n"
  "  --delta_fac <factor of delta>\n"
  "  --pf_exp <exponent of failure probability>\n"
  "  --index_ratio <ratio of index size>\n"
  "  --inacc_ratio <ratio of index inaccuracy>\n"
  "  --round <round of exact method>\n"
  "  --workloads <list of workloads>\n"
  "  --output\n";

struct {
  double alpha = 0.2;
  double beta = 1.0;
  double theta = 0.5;
  double eps = 0.5;
  double det_exp = 1.0;
  double det_fac = 1.0;
  double pf_exp = 1.0;
} config;

struct  {
  size_t round = 160;
  double alpha = 0.2;
} exact_config;

#define filepath(file) (file_path(2, argv[2], file))
#define workload_path(workload) \
  (file_path(3, argv[2], "workloads", workload.c_str()))
#define result_folder(workload) \
  (file_path(4, argv[2], "results", argv[1], workload.c_str()))
#define result_path(workload, node) \
  (file_path(2, result_folder(workload).c_str(), std::to_string(node).c_str()))

fora_interface *g;

void build_graph(char* argv[]) {
  fprintf(stdout, "loading meta data\n");
  auto [n, m, directed] = load_file<graph_meta>(filepath("meta"));
  fprintf(stdout, "n = %zu, m = %zu, %s\n", (size_t)n, (size_t)m,
    directed ? "directed" : "undirected");
  fflush(stdout);

  fprintf(stdout, "loading base graph\n");
  fflush(stdout);
  auto edges = load_file<edge_list>(filepath("graph_base"));

  fprintf(stdout, "building base graph\n");
  fflush(stdout);
  if (strcmp(argv[1], "exact") == 0) {
    g = new exact_ppr(directed, n, edges, exact_config);
  } else if (strcmp(argv[1], "fora") == 0) {
    g = new fora<windex_realtime>(directed, n, edges, config);
  } else if (strcmp(argv[1], "fora+") == 0) {
    g = new fora<windex_eager>(directed, n, edges, config);
  } else if (strcmp(argv[1], "agenda") == 0) {
    g = new fora<windex_lazy<true>>(directed, n, edges, config);
  } else if (strcmp(argv[1], "agenda*") == 0) {
    g = new fora<windex_lazy<false>>(directed, n, edges, config);
  } else if (strcmp(argv[1], "firm") == 0) {
    g = new fora<windex_inc>(directed, n, edges, config);
  } else {
    log_fatal("unknown scheme %s\nusage:\n%s\n", argv[1], help);
    exit(-1);
  }
}

void handle_workload(char* argv[], std::string workload, bool output) {
  fprintf(stdout, "handling workload %s\n", workload.c_str());
  fflush(stdout);
  Timer::reset_all();

  save_file(
    file_path(2, result_folder(workload).c_str(), "meta_configs"),
    g->experiment_configs());

  auto w = load_file<std::vector<update>>(workload_path(workload));
  for (auto [o, u, v] : w) {
    if (o == '?') {
      node_id s = u, k = v;
      log_info("querying source %zu", (size_t)s);
      if (!k) {
        auto outputer =
          [output, argv, workload, s] (const std::vector<double>& ppr) {
            if (output) save_file(result_path(workload, s), ppr);
          };
        g->evaluate_full(s, outputer);
      } else {
        auto outputer =
          [output, argv, workload, s] (const std::vector<node_id>& knodes) {
            if (output) save_file(result_path(workload, s), knodes);
          };
        g->evaluate_topk(s, k, outputer);
      }
    } else if (o == '+') {
      log_info("inserting edge %zu %zu", (size_t)u, (size_t)v);
      g->insert_edge(u, v);
    } else if (o == '-') {
      log_info("deleting edge %zu %zu", (size_t)u, (size_t)v);
      g->delete_edge(u, v);
    } else {
      log_error("unknown operation %c", o);
    }
  }

  fprintf(stdout, "time for updates: %lf\n", Timer::used(TIMER::UPDATE));
  fprintf(stdout, "time for queries: %lf"
    "(adapt: %lf, push: %lf, refine: %lf, check: %lf)\n",
    Timer::used(TIMER::EVALUATE),
    Timer::used(TIMER::ADAPT),
    Timer::used(TIMER::PUSH),
    Timer::used(TIMER::REFINE),
    Timer::used(TIMER::CHECK_K));
  fprintf(stdout, "time for output: %lf\n", Timer::used(TIMER::OUTPUT));
  fflush(stdout);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "missing argument <algo_name>\nusage:\n%s\n", help);
    return -1;
  }
  if (argc < 3) {
    fprintf(stderr, "missing argument <data_path>\nusage:\n%s\n", help);
    return -1;
  }

  bool output = false;
  std::vector<std::string> workloads;
  for (int i = 3; i < argc; ++i) {
    if (strcmp(argv[i], "--alpha") == 0) {
      config.alpha = exact_config.alpha = atof(argv[++i]);
      if (config.alpha <= 0 || config.alpha >= 1) {
        fprintf(stderr, "invalid alpha, must be in (0,1)\n");
        return -1;
      }
    } else if (strcmp(argv[i], "--epsilon") == 0) {
      config.eps = atof(argv[++i]);
      if (config.eps <= 0 || config.eps >= 1) {
        fprintf(stderr, "invalid epsilon, must be in (0,1)\n");
        return -1;
      }
    } else if (strcmp(argv[i], "--delta_exp") == 0) {
      config.det_exp = atof(argv[++i]);
      if (config.det_exp < 0) {
        fprintf(stderr, "invalid delta_exp, must be non-negative\n");
        return -1;
      }
    } else if (strcmp(argv[i], "--delta_fac") == 0) {
      config.det_fac = atof(argv[++i]);
      if (config.det_fac < 0) {
        fprintf(stderr, "invalid delta_fac, must be non-negative\n");
        return -1;
      }
    } else if (strcmp(argv[i], "--pf_exp") == 0) {
      config.pf_exp = atof(argv[++i]);
      if (config.pf_exp <= 0) {
        fprintf(stderr, "invalid pf_exp, must be positive\n");
        return -1;
      }
    } else if (strcmp(argv[i], "--index_ratio") == 0) {
      config.beta = atof(argv[++i]);
      if (config.beta <= 0) {
        fprintf(stderr, "invalid index_ratio, must be positive\n");
        return -1;
      }
    } else if (strcmp(argv[i], "--inacc_ratio") == 0) {
      config.theta = atof(argv[++i]);
      if (config.theta <= 0 || config.theta >= 1) {
        fprintf(stderr, "invalid inacc_ratio, must be in (0,1)\n");
        return -1;
      }
    } else if (strcmp(argv[i], "--round") == 0) {
      exact_config.round = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--workloads") == 0) {
      workloads = split(std::string(argv[++i]), ",");
    } else if (strcmp(argv[i], "--output") == 0) {
      output = true;
    } else {
      fprintf(stderr, "unknown option %s\nusage:\n%s\n", argv[i], help);
      return -1;
    }
  }

  build_graph(argv);
  for (std::string& workload : workloads)
    handle_workload(argv, workload, output);

  return 0;
}
