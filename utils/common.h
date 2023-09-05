#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <vector>
#include <map>
#include "omp.h"
#include <atomic>
#include <cmath>

#include "graph.h"
#include "split_huge_page.h"

#define OUTPUT_RET

#define BFS 1
#define SSSP 2
#define PAGERANK 3

#define HUGE_PAGE_SIZE 2097152
#define MAX_EDGES 4094967295
#define WEIGHT_MAX 1073741823
#define DYNAMIC_PROMOTION 21
#define PROMOTION_FIXED 2
#define MADVISE_ALL 50

#define MAX_BUFFER_SIZE 5000
#define APP_TIME 1000000
#define THP_TIME 3000000
#define SLEEP_TIME 1000
#define PROMOTION_SLEEP_TIME 30000000
#define KB_SIZE 1024
#define THP_SIZE_KB 2048
#define PERCENT_INTERVAL 0.05
#define MAX_NUM_BITS 16111

using namespace std;

int app = 0;
const char *done_filename = "done.txt", *other_filename;
string smaps_filename, stat_filename;
int pid, pidfd;

double user_time1, kernel_time1, user_time2, kernel_time2;
chrono::time_point<chrono::system_clock> clock_start, clock_end;
int rusage_out = 0;
struct rusage usage1, usage2;
vector<void*> data_structs;

unordered_map<string, unsigned long*> mem_regions;
unordered_map<string, vector<pair<unsigned long, unsigned long>>> promotion_regions;

void pin_start() {}
void pin_end() {}
void get_footprint() {}

void setup_app(const char* new_filename="") {
  // Signal to other processes that app execution will start
  const char* filename;
  if (new_filename != "") filename = new_filename;
  else filename = done_filename;
  ofstream output(filename);
  output.close();
  
  /*while (access(other_filename, F_OK) != 0) {
    sleep(0.5);
  }*/

  usleep(APP_TIME);

  // First rusage
  rusage_out = getrusage(RUSAGE_SELF, &usage1);
  if (rusage_out != 0) perror("Error with getrusage!");

  // Execute app
  printf("\n\nstarting kernel\n");
  clock_start = chrono::system_clock::now();
  get_times(stat_filename.c_str(), &user_time1, &kernel_time1);

  pin_start();
}

void cleanup_app(csr_graph G, const char* new_filename="", unsigned long* ret=NULL, double* ret_double=NULL) {
  pin_end();
  
  get_times(stat_filename.c_str(), &user_time2, &kernel_time2);
  clock_end = std::chrono::system_clock::now();
  printf("ending kernel\n");

  // Second rusage
  rusage_out = getrusage(RUSAGE_SELF, &usage2);
  if (rusage_out != 0) perror("Error with getrusage!");

  // Measure execution time
  chrono::duration<double> elapsed_seconds = clock_end-clock_start;
  printf("\ntotal kernel computation time: %f\n", elapsed_seconds.count());
  printf("user time: %f\nkernel time: %f\n" , (user_time2-user_time1), (kernel_time2-kernel_time1));
  printf("footprint: start = %luKB, end = %luKB, diff = %luKB\npage reclaims: %lu\npage faults: %lu\nswaps: %lu\n", usage1.ru_maxrss, usage2.ru_maxrss, (usage2.ru_maxrss-usage1.ru_maxrss), (usage2.ru_minflt-usage1.ru_minflt), (usage2.ru_majflt-usage1.ru_majflt), (usage2.ru_nswap-usage1.ru_nswap));
  fflush(stdout);

#if defined(OUTPUT_RET)
  if (ret != NULL) {
    printf("Writing output to file!\n");
    ofstream outfile;
    outfile.open ("out.txt");
    for (int i = 0; i < G.nodes; i++) {
      outfile << ret[i] << "\n";
    }
    outfile.close();
  }

  if (ret_double != NULL) {
    printf("Writing output to file!\n");
    ofstream outfile;
    outfile.open ("out.txt");
    for (int i = 0; i < G.nodes; i++) {
      outfile << ceil(ret_double[i]*1000.0)/1000.0 << "\n";
    }
    outfile.close();
  }
#endif

  // Clean up
  for (void* ptr : data_structs) {
    free(ptr);
  }
  clean_csr_graph(G);
          
  const char* filename;
  if (new_filename != "") filename = new_filename;
  else filename = done_filename;
  remove(filename);
}

// Perf thread
void launch_perf(int cpid, const char* perf_cmd, const char* new_filename="") {
  struct stat done_buffer;
  char buf[MAX_BUFFER_SIZE];
  sprintf(buf, perf_cmd, pid);
  
  const char* filename;
  if (new_filename != "") filename = new_filename;
  else filename = done_filename;

  printf("Perf process spawned! cpid %d waiting to run %s\n", cpid, buf);
  while (stat (filename, &done_buffer) != 0 || stat (done_filename, &done_buffer) != 0) {
    usleep(SLEEP_TIME);
  }
  printf("cpid %d running %s\n", cpid, buf);

  int err = execl("/bin/sh", "sh", "-c", buf, NULL);
  if (err == -1) printf("Error with perf!\n");
  fflush(stdout);
}

// THP tracking thread
void launch_thp_tracking(int cpid, const char* thp_filename, const char* pf_filename, const char* new_filename="") {
  struct stat done_buffer, buffer;
  string cmd;
  vector<tuple<double, unsigned long, unsigned long>> thps_over_time, page_faults_over_time;
  unsigned long anon_size, thp_size;
  int soft_pf1, hard_pf1, soft_pf2, hard_pf2, soft_pf, hard_pf;

  const char* filename;
  if (new_filename != "") filename = new_filename;
  else filename = done_filename;
  
  setup_pagemaps(pid);

  printf("THP tracking process spawned! cpid %d waiting to run\n", cpid);
  while (stat (filename, &done_buffer) != 0 || stat (done_filename, &done_buffer) != 0) {
    usleep(SLEEP_TIME);
  }
  printf("cpid %d running\n", cpid);
  fflush(stdout);

  get_pfs(stat_filename.c_str(), &soft_pf1, &hard_pf1);

  auto glob_start = chrono::system_clock::now();
  //while(stat (smaps_filename.c_str(), &buffer) == 0) {
  while(0 == kill(pid, 0)) {
    // Track # THPs over time
    auto end = std::chrono::system_clock::now();
    chrono::duration<double> elapsed_seconds = end-glob_start;

    anon_size = 0;
    thp_size = 0;
    tuple<unsigned long, unsigned long> data = read_smap_data(smaps_filename.c_str());
    thp_size = get<0>(data);
    anon_size = get<1>(data);

    if (anon_size == -1 || thp_size == -1) break;
    thps_over_time.push_back(make_tuple(elapsed_seconds.count(), anon_size, thp_size));

    get_pfs(stat_filename.c_str(), &soft_pf2, &hard_pf2);
    soft_pf = soft_pf2 - soft_pf1;
    hard_pf = hard_pf2 - hard_pf1;
    page_faults_over_time.push_back(make_tuple(elapsed_seconds.count(), soft_pf, hard_pf));

    usleep(THP_TIME);
  }

  ofstream thp_file(thp_filename);
  for (tuple<double, unsigned long, unsigned long> data : thps_over_time) {
    thp_file << to_string(get<0>(data)) + " sec: " + to_string(get<1>(data)) + " anon, " + to_string(get<2>(data)) + " THPs\n";
  }
  thp_file.close();

  ofstream pf_file(pf_filename);
  for (tuple<double, unsigned long, unsigned long> data : page_faults_over_time) {
    pf_file << to_string(get<0>(data)) + " sec: " + to_string(get<1>(data)) + " soft pf, " + to_string(get<2>(data)) + " hard pf\n";
  }
  pf_file.close();

  return;
}

void read_promotions_over_time(map<unsigned long, vector<pair<unsigned long, unsigned long>>>* data, const char* filename) {
  char* line = NULL;
  size_t len = 0, num_lines = 0;
  string region_name;
  unsigned long start, tmp_time, time, tmp_offset, offset, num_offsets = 0;
  int err;

  FILE* fp = fopen(filename, "r");
  if (fp == NULL) {
    cout << "Error reading " << filename << endl;
    exit(EXIT_FAILURE);
  } else {
    cout << "Reading " << filename << endl;
  }

  while ((getline(&line, &len, fp)) != -1) {
    num_lines++;

    string line_str(line);
    line_str.erase(remove(line_str.begin(), line_str.end(), '\n'), line_str.end());

    stringstream ss(line_str);
    vector<string> result;
    while(ss.good()) {
      string substr;
      getline(ss, substr, ',');
      result.push_back(substr);
    }

    tmp_time = stoul(result[0], NULL, 0);
    if (tmp_time != time) {
      // new promotion time, restart range counting
      if (num_offsets > 0) (*data)[time].push_back(make_pair(start, num_offsets*pmd_pagesize)); 
      num_offsets = 0;
      offset = -1;
    }
    time = tmp_time;

    tmp_offset = stoul(result[1], NULL, 0) * pmd_pagesize; // current offset
    if (tmp_offset == offset+pmd_pagesize || num_offsets == 0) { // continous range or first offset
      if (num_offsets == 0) start = tmp_offset;
      num_offsets++;
    } else { // end of continuous range
      (*data)[time].push_back(make_pair(start, num_offsets*pmd_pagesize)); // end of continuous range

      // information for next range
      start = tmp_offset;
      num_offsets = 1;
    }
    offset = tmp_offset;
  }
  if (num_lines > 0) (*data)[time].push_back(make_pair(start, num_offsets*pmd_pagesize));

  /*
  map<unsigned long, vector<pair<unsigned long, unsigned long>>>::iterator it;
  for (it = data->begin(); it != data->end(); it++) {
    cout << "\n\nTIME: " << it->first << endl;
    vector<pair<unsigned long, unsigned long>> madvise_ranges = it->second;
    for (unsigned long m = 0; m < madvise_ranges.size(); m++) {
      cout << madvise_ranges[m].first/pmd_pagesize << " " << madvise_ranges[m].second/pmd_pagesize << endl;
    }
  }
  cout << endl;
  */

  fclose(fp);
}

// Promotion thread
void launch_promoter(int cpid, const char* promotion_filename, const char* demotion_filename="", const char* new_filename="") {
  struct stat done_buffer;
  unsigned long time, base, offset;
  struct iovec iov;
  int err;

  const char* filename;
  if (new_filename != "") filename = new_filename;
  else filename = done_filename;

  map<unsigned long, vector<pair<unsigned long, unsigned long>>> promotions_over_time;
  map<unsigned long, vector<pair<unsigned long, unsigned long>>> demotions_over_time;
  map<unsigned long, vector<pair<unsigned long, unsigned long>>>::iterator promotion_it;
  map<unsigned long, vector<pair<unsigned long, unsigned long>>>::iterator demotion_it;

  setup_pagemaps(pid);
  if (promotion_filename != "") read_promotions_over_time(&promotions_over_time, promotion_filename);
  if (demotion_filename != "") read_promotions_over_time(&demotions_over_time, demotion_filename);

  for (promotion_it = promotions_over_time.begin(); promotion_it != promotions_over_time.end(); promotion_it++) {
    cout << "PROMOTION KEY: " << promotion_it->first << endl;
    fflush(stdout);
  }
  for (demotion_it = demotions_over_time.begin(); demotion_it != demotions_over_time.end(); demotion_it++) {
    cout << "DEMOTION KEY: " << demotion_it->first << endl;
    fflush(stdout);
  }

  promotion_it = promotions_over_time.begin();
  demotion_it = demotions_over_time.begin();

  printf("Promotion/demotion process spawned! cpid %d waiting to run\n", cpid);
  while (stat (filename, &done_buffer) != 0 || stat (done_filename, &done_buffer) != 0) {
    usleep(SLEEP_TIME);
  }
  printf("cpid %d running\n", cpid);
  fflush(stdout);

  usleep(APP_TIME);

  vector<pair<unsigned long, unsigned long>> madvise_ranges;
  while(0 == kill(pid, 0)) {
    time = promotion_it->first;
    madvise_ranges = promotions_over_time[time];

    for (unsigned long m = 0; m < madvise_ranges.size(); m++) {
      base = madvise_ranges[m].first/pmd_pagesize;
      offset = madvise_ranges[m].second/pmd_pagesize;

      iov.iov_base = (char*) 0 + base*pmd_pagesize;
      iov.iov_len = offset*pmd_pagesize;
      cout << "PROMOTING: " << (unsigned long) base << " " << offset;
      fflush(stdout);
      err = syscall(SYS_process_madvise, pidfd, &iov, 1, MADV_PROMOTE, 0);
      if (err < 0) perror("Error!");
      else cout << " --> SUCCESSFUL!" << endl;
    }
    if (time == demotion_it->first) { // demotion takes place
      madvise_ranges = demotions_over_time[time];
      for (unsigned long m = 0; m < madvise_ranges.size(); m++) {
        base = madvise_ranges[m].first/pmd_pagesize;
        offset = madvise_ranges[m].second/pmd_pagesize;

        iov.iov_base = (char*) 0 + base*pmd_pagesize;
        iov.iov_len = offset*pmd_pagesize;
        cout << "DEMOTING: " << (unsigned long) base << " " << offset;
        fflush(stdout);
        err = syscall(SYS_process_madvise, pidfd, &iov, 1, MADV_DEMOTE, 0);
        if (err < 0) perror("Error!");
        else cout << " --> SUCCESSFUL!" << endl;
      }
      demotion_it++;
    }

    promotion_it++;
    if (promotion_it == promotions_over_time.end()) break;
    usleep(PROMOTION_SLEEP_TIME);
  }
  fflush(stdout);
  
  return;
}

// Set up irregularly accessed data
void create_irreg_data(int run_kernel, unsigned long num_nodes, unsigned long** ret) {
  void *tmp = nullptr;
  unsigned long num_thp_nodes;

  //posix_memalign(&tmp, 1 << 21, num_nodes * sizeof(unsigned long));
  //*ret = static_cast<unsigned long*>(tmp);
  *ret = (unsigned long*) malloc(num_nodes * sizeof(unsigned long));
 
  if (run_kernel >= 1 && run_kernel <= 20) {
    num_thp_nodes = (unsigned long) num_nodes*run_kernel*PERCENT_INTERVAL;
    cout << "Applying THPs to " << num_thp_nodes << "/" << num_nodes << endl;

    int err = madvise(*ret, num_thp_nodes * sizeof(unsigned long), MADV_HUGEPAGE);
    if (err != 0) perror("Error!");
    else cout << "MADV_HUGEPAGE ret successful!" << endl;
  } else if (run_kernel == PROMOTION_FIXED) {
    unsigned long offset, size;
    int err;

    cout << "PROP ARRAY:" << endl;
    vector<pair<unsigned long, unsigned long>> madvise_ranges = promotion_regions["PROP_ARRAY"];
    for (unsigned long m = 0; m < madvise_ranges.size(); m++) {
      offset = madvise_ranges[m].first;
      size = madvise_ranges[m].second;
      cout << offset/pmd_pagesize << " " << size/pmd_pagesize;
      err = madvise((unsigned long*)((char*)*ret + offset), size, MADV_HUGEPAGE);
      if (err != 0) perror("Error!");
      else cout << " --> SUCCESSFUL!" << endl;
    }
  } else if (run_kernel == MADVISE_ALL) {
    int err = madvise(*ret, num_nodes * sizeof(unsigned long), MADV_HUGEPAGE);
    if (err != 0) perror("Error!");
    else cout << "MADV_HUGEPAGE ret successful!" << endl;
  }

  fflush(stdout);
}

// Set up irregularly accessed data
void create_irreg_data_pr(int run_kernel, unsigned long num_nodes, double** ret) {
  void *tmp = nullptr;
  unsigned long num_thp_nodes;

  //posix_memalign(&tmp, 1 << 21, num_nodes * sizeof(double));
  //*ret = static_cast<double*>(tmp);
  *ret = (double*) malloc(num_nodes * sizeof(double));

  if (run_kernel >= 1 && run_kernel <= 20) {
    num_thp_nodes = (unsigned long) num_nodes*run_kernel*PERCENT_INTERVAL;
    cout << "Applying THPs to " << num_thp_nodes << "/" << num_nodes << endl;

    int err = madvise(*ret, num_thp_nodes * sizeof(double), MADV_HUGEPAGE);
    if (err != 0) perror("Error!");
    else cout << "MADV_HUGEPAGE ret successful!" << endl;
  } else if (run_kernel == PROMOTION_FIXED) {
    unsigned long offset, size;
    int err;

    cout << "PROP ARRAY:" << endl;
    vector<pair<unsigned long, unsigned long>> madvise_ranges = promotion_regions["PROP_ARRAY"];
    for (unsigned long m = 0; m < madvise_ranges.size(); m++) {
      offset = madvise_ranges[m].first;
      size = madvise_ranges[m].second;
      cout << offset/pmd_pagesize << " " << size/pmd_pagesize;
      err = madvise((double*)((char*)*ret + offset), size, MADV_HUGEPAGE);
      if (err != 0) perror("Error!");
      else cout << " --> SUCCESSFUL!" << endl;
    }
  } else if (run_kernel == MADVISE_ALL) {
    int err = madvise(*ret, num_nodes * sizeof(double), MADV_HUGEPAGE);
    if (err != 0) perror("Error!");
    else cout << "MADV_HUGEPAGE ret successful!" << endl;
  }

  fflush(stdout);
}

csr_graph parse_bin_files(string base, int run_kernel=0, int edge_values=0) {
  csr_graph ret;
  ifstream nodes_edges_file(base + "num_nodes_edges.txt");
  unsigned long nodes, edges;
  int err;
  void *tmp = nullptr;
  auto start = chrono::system_clock::now();
  FILE *fp;

  nodes_edges_file >> nodes;
  nodes_edges_file >> edges;
  nodes_edges_file.close();
  cout << "Nodes: " << nodes << ", Edges: " << edges << "\n";

  ret.nodes = nodes;
  ret.edges = edges;
  
  /*posix_memalign(&tmp, 1 << 21, (ret.nodes+1) * sizeof(unsigned long));
  ret.node_array = static_cast<unsigned long*>(tmp);

  posix_memalign(&tmp, 1 << 21, ret.edges * sizeof(unsigned long));
  ret.edge_array = static_cast<unsigned long*>(tmp);

  if (edge_values == 0) {
    posix_memalign(&tmp, 1 << 21, ret.edges * sizeof(weightT));
    ret.edge_values = static_cast<weightT*>(tmp);
  }*/

  ret.node_array = (unsigned long*) malloc((ret.nodes+1) * sizeof(unsigned long));
  ret.edge_array = (unsigned long*) malloc(ret.edges * sizeof(unsigned long));
  if (edge_values == 0) ret.edge_values = (weightT*) malloc(ret.edges * sizeof(weightT));

  if (run_kernel == PROMOTION_FIXED) {
    unsigned long offset, size, *addr;
    string region;
    vector<pair<unsigned long, unsigned long>> madvise_ranges;
    
    unordered_map<string, unsigned long*> regions_map = {
	{"NODE_ARRAY", ret.node_array}, {"EDGE_ARRAY", ret.edge_array}, {"VALS_ARRAY", ret.edge_values}};

    for (auto entry : regions_map) {
      region = entry.first;
      addr = entry.second;
      cout << region << endl;
    
      if (region == "VALS_ARRAY" && edge_values != 0) continue;
  
      madvise_ranges = promotion_regions[region];
      for (unsigned long m = 0; m < madvise_ranges.size(); m++) {
        offset = madvise_ranges[m].first;
        size = madvise_ranges[m].second;
        cout << offset/pmd_pagesize << " " << size/pmd_pagesize;
        err = madvise((unsigned long*)((char*)addr + offset), size, MADV_HUGEPAGE);
        if (err != 0) perror("Error!");
        else cout << " --> SUCCESSFUL!" << endl;
      }
    }
  }

  if (run_kernel == 200 || run_kernel == MADVISE_ALL) {
    err = madvise(ret.node_array, ret.nodes * sizeof(unsigned long), MADV_HUGEPAGE);
    if (err != 0) perror("Error!");
    else cout << "madvise node array successful!" << endl;
  }
  if (run_kernel == 300 || run_kernel == MADVISE_ALL) {
    err = madvise(ret.edge_array, ret.edges * sizeof(unsigned long), MADV_HUGEPAGE);
    if (err != 0) perror("Error!");
    else cout << "madvise edge array successful!" << endl;
  }
  if (edge_values == 0 && (run_kernel == 400 || run_kernel == MADVISE_ALL)) {
    err = madvise(ret.edge_values, ret.edges * sizeof(unsigned long), MADV_HUGEPAGE);
    if (err != 0) perror("Error!");
    else cout << "madvise edge values array successful!" << endl;
  } 

  // ***** NODE ARRAY *****
  fp = fopen((base + "node_array.bin").c_str(), "rb");

  if (ret.edges > MAX_EDGES) {
    cout << "reading byte length of:    " << (ret.nodes + 1) * sizeof(unsigned long) << endl;
    for (unsigned long n = 0; n < ret.nodes + 1; n++) {
      fread(&ret.node_array[n], sizeof(unsigned long), 1, fp);
    }
  } else {
    cout << "reading byte length of:    " << (ret.nodes + 1) * sizeof(unsigned int) << endl;

    for (unsigned long n = 0; n < ret.nodes + 1; n++) {
      fread(&ret.node_array[n], sizeof(unsigned int), 1, fp);
    }
  }

  fclose(fp);
  ret.node_array[0] = 0;

  // ***** EDGE ARRAY *****
  fp = fopen((base + "edge_array.bin").c_str(), "rb");

  if (ret.edges > MAX_EDGES) {
    cout << "reading byte length of:    " << (ret.edges) * sizeof(unsigned long) << endl;

    for (unsigned long e = 0; e < ret.edges; e++) {
      fread(&ret.edge_array[e], sizeof(unsigned long), 1, fp);
    }
  } else {
    cout << "reading byte length of:    " << (ret.edges) * sizeof(unsigned int) << endl;

    for (unsigned long e = 0; e < ret.edges; e++) {
      fread(&ret.edge_array[e], sizeof(unsigned int), 1, fp);
    }
  }

  fclose(fp);

  // ***** VALUES ARRAY *****
  if (edge_values == 0) {
    fp = fopen((base + "edge_values.bin").c_str(), "rb");

    if (ret.edges > MAX_EDGES) {
      cout << "reading byte length of:    " << (ret.edges) * sizeof(weightT) << endl;

      for (unsigned long e = 0; e < ret.edges; e++) {
        fread(&ret.edge_values[e], sizeof(weightT), 1, fp);
      }
    } else {
      cout << "reading byte length of:    " << (ret.edges) * sizeof(unsigned int) << endl;

      for (unsigned long e = 0; e < ret.edges; e++) {
        fread(&ret.edge_values[e], sizeof(unsigned int), 1, fp);
      }
    }

    fclose(fp);
  }

  auto end = std::chrono::system_clock::now();
  chrono::duration<double> elapsed_seconds = end-start;
  cout << "Reading graph elapsed time: " << elapsed_seconds.count() << "s\n";
  fflush(stdout);

  return ret;
}
