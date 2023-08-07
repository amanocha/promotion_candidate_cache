#include "../../../utils/common.h"
#include "pagerank.h"

using namespace std;
extern int errno;

void launch_app(string graph_fname) {
  csr_graph G;
  unsigned long in_index, out_index, *in_wl, *out_wl;
  double *x, *in_r, *out_r;
  unsigned int run_kernel = 0;

  // Initialize data and create irregular data
  G = parse_bin_files(graph_fname, run_kernel, 1);
  create_irreg_data_pr(run_kernel, G.nodes, &out_r);
  init_pagerank(run_kernel, G, &x, &in_r, &out_r, &in_wl, &in_index, &out_wl, &out_index);
  print_regions(G, x, in_r, out_r, in_wl, out_wl);
  
  data_structs.push_back((void*) x);
  data_structs.push_back((void*) in_r);
  data_structs.push_back((void*) out_r);
  data_structs.push_back((void*) in_wl);
  data_structs.push_back((void*) out_wl);

  setup_app();

  pagerank(G, x, in_r, out_r, in_wl, &in_index, out_wl, &out_index, 0, 1); // part of program to perf stat
  
  cleanup_app(G);
}

int main(int argc, char** argv) {
  string graph_fname;

  assert(argc >= 2);
  graph_fname = argv[1];
    
  pid = getpid();
  stat_filename = "/proc/" + to_string(pid) + "/stat";

  launch_app(graph_fname);

  return 0;
}
