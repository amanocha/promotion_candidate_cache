#include "../../../../utils/common.h"
#include "pagerank.h"

using namespace std;
extern int errno;

void launch_app(string graph_fname, unsigned long num_threads_arg) {
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

  omp_set_num_threads(num_threads_arg);
  setup_app();

#pragma omp parallel
{
  int tid = omp_get_thread_num();
  int num_threads = omp_get_num_threads();
  cout << tid << " running!" << endl;
  pagerank(G, x, in_r, out_r, in_wl, &in_index, out_wl, &out_index, tid, num_threads); // part of program to perf stat
}
  
  cleanup_app(G);
}

int main(int argc, char** argv) {
  string graph_fname;
  unsigned long start_seed = 0, num_threads = 2;

  assert(argc >= 2);
  graph_fname = argv[1];
  if (argc >= 3) start_seed = atoi(argv[2]);
  if (argc >= 4) num_threads = atoi(argv[3]);
    
  pid = getpid();
  stat_filename = "/proc/" + to_string(pid) + "/stat";

  launch_app(graph_fname, num_threads);

  return 0;
}
