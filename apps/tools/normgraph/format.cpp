#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <tuple>
#include "apps/io/file.hpp"
#include "apps/types.hpp"

constexpr char help[] =
  "format <data_path> [options]\n"
  "options:\n"
  "  --directed|undirected\n";

#define filepath(file) (file_path(2, argv[1], file))

int main(int argc, char *argv[]) {
  if (argc < 2) {
    log_fatal("missing argument <data_path>\nusage:\n%s", help);
    return -1;
  }

  bool dird = true;
  for (int i = 2; i < argc; ++i) {
    if (strcmp(argv[i], "--directed") == 0) {
      dird = true;
    } else if (strcmp(argv[i], "--undirected") == 0) {
      dird = false;
    } else {
      log_fatal("unknown option %s\nusage:\n%s", argv[i], help);
      return -1;
    }
  }

  std::ifstream f_in(filepath("text"));
  if (f_in.eof() || f_in.fail()) {
    log_fatal("cannot open file '%s'", filepath("text").c_str());
    return -1;
  }

  node_id n = 0;
  edge_list edges;
  for (size_t u, v; f_in >> u >> v; ) {
    n = std::max(n, (node_id)std::max(u, v) + 1);
    if (dird)
      edges.push_back(std::make_pair(u + 1, v + 1));
    else
      edges.push_back(std::make_pair(std::min(u, v) + 1, std::max(u, v) + 1));
  }
  f_in.close();

  std::sort(edges.begin(), edges.end());
  edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
  edge_id m = edges.size();

  save_file(filepath("meta"), std::make_tuple(n, m, dird));
  save_file(filepath("graph"), edges);

  return 0;
}
