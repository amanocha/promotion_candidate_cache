#include "../../utils/common.h"
#include "reuse.h"
#include "bfs/bfs.h"
#include "sssp/sssp.h"
#include "pagerank/pagerank.h"

using namespace std;
extern int errno;

void launch_app(string graph_fname, string app, int page_size, unsigned long start_seed) {
  csr_graph G;
  unsigned long *ret, in_index, out_index, *in_wl, *out_wl;
  double *x, *in_r, *out_r;
  double user_time1, kernel_time1, user_time2, kernel_time2;
  chrono::time_point<chrono::system_clock> start, end;
  int rusage_out = 0;
  struct rusage usage1, usage2;
  bool edge_values = (app == "sssp") ? true : false;

  REUSE_PAGE_SIZE = page_size;

  // Initialize data and create irregular data
  G = parse_bin_files(graph_fname, 0, edge_values);
  
  if (app == "pagerank") {
    create_irreg_data_pr(0, G.nodes, &out_r);
    init_pagerank(G, &x, &in_r, &out_r, &in_wl, &in_index, &out_wl, &out_index);
  } else {
    create_irreg_data(0, G.nodes, &ret);
    if (app == "sssp") {
      init_sssp(G.nodes, start_seed, &in_index, &out_index, &in_wl, &out_wl, ret);
    } else {
      init_bfs(G.nodes, start_seed, &in_index, &out_index, &in_wl, &out_wl, ret);
    }
  }
  update_mem_regions(G, ret, in_wl, out_wl, &in_index, &out_index);

  // First rusage
  rusage_out = getrusage(RUSAGE_SELF, &usage1);
  if (rusage_out != 0) perror("Error with getrusage!");

  // Execute app
  printf("\n\nstarting kernel\n");
  start = chrono::system_clock::now();
  
  if (app == "pagerank") {
    pagerank(G, x, in_r, out_r, in_wl, &in_index, out_wl, &out_index, 0, 1); // part of program to perf stat
  } else if (app == "sssp") {
    sssp(G, ret, in_wl, &in_index, out_wl, &out_index, 0, 1); // part of program to perf stat
  } else {
    bfs(G, ret, in_wl, &in_index, out_wl, &out_index, 0 , 1); // part of program to perf stat
  }

  end = std::chrono::system_clock::now();
  printf("ending kernel\n");

#ifdef TRACK_ACCESS
  summarize();
#endif

  // Second rusage
  rusage_out = getrusage(RUSAGE_SELF, &usage2);
  if (rusage_out != 0) perror("Error with getrusage!");

  // Measure execution time
  chrono::duration<double> elapsed_seconds = end-start;
  printf("\ntotal kernel computation time: %f\n", elapsed_seconds.count());
  printf("user time: %f\nkernel time: %f\n" , (user_time2-user_time1), (kernel_time2-kernel_time1));
  printf("footprint: start = %luKB, end = %luKB, diff = %luKB\npage reclaims: %lu\npage faults: %lu\nswaps: %lu\n", usage1.ru_maxrss, usage2.ru_maxrss, (usage2.ru_maxrss-usage1.ru_maxrss), (usage2.ru_minflt-usage1.ru_minflt), (usage2.ru_majflt-usage1.ru_majflt), (usage2.ru_nswap-usage1.ru_nswap));
  fflush(stdout);

  // Clean up
  free(ret);
  free(in_wl);
  free(out_wl);
  clean_csr_graph(G);
}

int main(int argc, char** argv) {
  string graph_fname, app;
  unsigned long page_size = 4096, start_seed = 0;
  struct bitmask *parent_mask = NULL;

  assert(argc >= 3);
  graph_fname = argv[1];
  app = argv[2];
  if (argc >= 4) page_size = atoi(argv[3]);
  if (argc >= 5) start_seed = atoi(argv[4]);

  // Run app process
  launch_app(graph_fname, app, page_size, start_seed);

  return 0;
}
