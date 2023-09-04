#include "../../../utils/common.h"
#include "pagerank.h"

using namespace std;
extern int errno;

void launch_app(string graph_fname, int run_kernel) {
  csr_graph G;
  unsigned long in_index, out_index, *in_wl, *out_wl;
  double *x, *in_r, *out_r;
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
  int run_kernel = 0;
  const char *perf_cmd, *thp_filename = "thp.txt", *pf_filename = "pf.txt", *promotion_filename = "", *demotion_filename = "";
  unsigned long max_demotion_scans = ULLONG_MAX;

  assert(argc >= 2);
  graph_fname = argv[1];
  if (argc >= 3) run_kernel = atoi(argv[2]);
  if (argc >= 4) perf_cmd = argv[3];
  else perf_cmd = "perf stat -p %d -B -v -e dTLB-load-misses,dTLB-loads -o stat.log";
  if (argc >= 5) thp_filename = argv[4];
  if (argc >= 6) pf_filename = argv[5];
  if (argc >= 7) promotion_filename = argv[6];
  if (argc >= 8) demotion_filename = argv[7];

  if (run_kernel >= 0) {
    pid = getpid();
    pidfd = syscall(SYS_pidfd_open, pid, 0);
    smaps_filename = "/proc/" + to_string(pid) + "/smaps";
    stat_filename = "/proc/" + to_string(pid) + "/stat";
    if (access(done_filename, F_OK) != -1) remove(done_filename);

    // FORK #1: Create perf process
    int cpid = fork();
    if (cpid == 0) {
      launch_perf(cpid, perf_cmd); // child process running perf
    } else {
      setpgid(cpid, 0); // set the child the leader of its process group

      // FORK #2: Create THP tracking process
      int cpid2 = fork();
      if (cpid2 == 0) {
        launch_thp_tracking(cpid2, thp_filename, pf_filename); // child process tracking THPs
      } else {
        setpgid(cpid2, 0); // set the child the leader of its process group

        int cpid3 = fork();
        if (cpid3 == 0) {
          if (run_kernel == DYNAMIC_PROMOTION) launch_promoter(cpid3, promotion_filename, demotion_filename);
        } else {
          setpgid(cpid3, 0); // set the child the leader of its process group

          launch_app(graph_fname, run_kernel);

          // kill child processes and all their descendants, e.g. sh, perf stat, etc.
          kill(-cpid, SIGINT); // stop perf stat
        }
      }
    }
  }

  return 0;
}
