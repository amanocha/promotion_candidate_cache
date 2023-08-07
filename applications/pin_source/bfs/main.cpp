#include "../../../utils/common.h"
#include "bfs.h"

using namespace std;
extern int errno;

void launch_app(string graph_fname, unsigned long start_seed) {
  csr_graph G;
  unsigned long *ret, in_index, out_index, *in_wl, *out_wl;

  // Initialize data and create irregular data
  G = parse_bin_files(graph_fname, 0, 1);
  create_irreg_data(0, G.nodes, &ret);
  init_bfs(G.nodes, start_seed, &in_index, &out_index, &in_wl, &out_wl, ret);
  print_regions(G, ret, in_wl, out_wl);
  data_structs.push_back((void*) ret);
  data_structs.push_back((void*) in_wl);
  data_structs.push_back((void*) out_wl);

  setup_app();
  
  bfs(G, ret, in_wl, &in_index, out_wl, &out_index, 0 , 1); // part of program to perf stat

  cleanup_app(G);
}

int main(int argc, char** argv) {
  string graph_fname;
  unsigned long start_seed = 0;

  assert(argc >= 2);
  graph_fname = argv[1];
  if (argc >= 3) start_seed = atoi(argv[2]);

  pid = getpid();
  stat_filename = "/proc/" + to_string(pid) + "/stat";

  // Run app process
  launch_app(graph_fname, start_seed);

  return 0;
}
