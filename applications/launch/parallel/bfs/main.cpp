#include "../../../../utils/common.h"
#include "bfs.h"

using namespace std;
extern int errno;

void launch_app(string graph_fname, int run_kernel, unsigned long start_seed, unsigned long num_threads_arg) {
  csr_graph G;
  unsigned long *ret, in_index, out_index, *in_wl, *out_wl;

  // Initialize data and create irregular data
  G = parse_bin_files(graph_fname, run_kernel, 1);
  create_irreg_data(run_kernel, G.nodes, &ret);
  init_bfs(run_kernel, G.nodes, start_seed, ret, &in_wl, &in_index, &out_wl, &out_index);
  print_regions(G, ret, in_wl, out_wl);
  data_structs.push_back((void*) ret);
  data_structs.push_back((void*) in_wl);
  data_structs.push_back((void*) out_wl);

  omp_set_num_threads(num_threads_arg);
  setup_app();

#pragma omp parallel
{
  int tid = omp_get_thread_num();
  int num_threads = omp_get_num_threads();
  cout << tid << " running!" << endl;
  bfs(G, ret, in_wl, &in_index, out_wl, &out_index, tid, num_threads); // part of program to perf stat
}

  cleanup_app(G);
}

int main(int argc, char** argv) {
  string graph_fname;
  unsigned long start_seed = 0, num_threads = 2;
  int run_kernel = 0;
  const char *perf_cmd, *thp_filename = "thp.txt", *pf_filename = "pf.txt", *promotion_filename = "";

  assert(argc >= 2);
  graph_fname = argv[1];
  if (argc >= 3) num_threads = atoi(argv[2]);
  if (argc >= 4) run_kernel = atoi(argv[3]);
  if (argc >= 5) start_seed = atoi(argv[4]);
  if (argc >= 6) perf_cmd = argv[5];
  else perf_cmd = "perf stat -p %d -B -v -e dTLB-load-misses,dTLB-loads -o stat.log";
  if (argc >= 7) thp_filename = argv[6];
  if (argc >= 8) pf_filename = argv[7];
  if (argc >= 9) promotion_filename = argv[8];

  if (run_kernel >= 0) {
    pid = getpid();
    pidfd = syscall(SYS_pidfd_open, pid, 0);
    smaps_filename = "/proc/" + to_string(pid) + "/smaps_rollup";
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
          if (run_kernel == HAWKEYE) launch_promoter(cpid3, promotion_filename);
        } else {
          setpgid(cpid3, 0); // set the child the leader of its process group

          launch_app(graph_fname, run_kernel, start_seed, num_threads);

          // kill child processes and all their descendants, e.g. sh, perf stat, etc.
          kill(-cpid, SIGINT); // stop perf stat
        }
      }
    }
  }

  return 0;
}
