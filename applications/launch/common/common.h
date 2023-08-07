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

#include "split_huge_page.h"

#define HUGE_PAGE_SIZE 2097152
#define PROMOTION_FIXED 21
#define HAWKEYE 22
#define MADVISE_ALL 50

#define MAX_BUFFER_SIZE 5000
#define APP_TIME 1000000
#define THP_TIME 3000000
#define WAIT_TIME 5000000
#define SLEEP_TIME 1000
#define PROMOTION_SLEEP_TIME 30000000
#define KB_SIZE 1024
#define THP_SIZE_KB 2048
#define PERCENT_INTERVAL 0.05
#define MAX_NUM_BITS 16111

using namespace std;

int app = 0;
const char* done_filename = "done.txt";
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

void setup_footprint() {
  // First rusage
  rusage_out = getrusage(RUSAGE_SELF, &usage1);
  if (rusage_out != 0) perror("Error with getrusage!");
}

void cleanup_footprint() {
  // Second rusage
  rusage_out = getrusage(RUSAGE_SELF, &usage2);
  if (rusage_out != 0) perror("Error with getrusage!");

  printf("footprint: start = %luKB, end = %luKB, diff = %luKB\npage reclaims: %lu\npage faults: %lu\nswaps: %lu\n", usage1.ru_maxrss, usage2.ru_maxrss, (usage2.ru_maxrss-usage1.ru_maxrss), (usage2.ru_minflt-usage1.ru_minflt), (usage2.ru_majflt-usage1.ru_majflt), (usage2.ru_nswap-usage1.ru_nswap));
  fflush(stdout);
}

void setup_app() {
  // Signal to other processes that app execution will start
  //ofstream output(done_filename);
  //output.close();
  //usleep(APP_TIME);

  // Execute app
  printf("\n\nstarting kernel\n");
  clock_start = chrono::system_clock::now();
  get_times(stat_filename.c_str(), &user_time1, &kernel_time1);
  fflush(stdout);

  pin_start();
}

void cleanup_app() {
  pin_end();
  
  get_times(stat_filename.c_str(), &user_time2, &kernel_time2);
  clock_end = std::chrono::system_clock::now();
  printf("ending kernel\n");

  // Measure execution time
  chrono::duration<double> elapsed_seconds = clock_end-clock_start;
  printf("\ntotal kernel computation time: %f\n", elapsed_seconds.count());
  printf("user time: %f\nkernel time: %f\n" , (user_time2-user_time1), (kernel_time2-kernel_time1));
  //printf("vm footprint: start = %luKB, end = %luKB, diff = %luKB\n", vs1, vs2, (vs2-vs1));
  //printf("rss: start = %luKB, end = %luKB, diff = %luKB\n", rss1, rss2, (rss2-rss1));
  fflush(stdout);
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
  fflush(stdout);
  while (stat (done_filename, &done_buffer) != 0 || stat (filename, &done_buffer) != 0) {
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
  vector<tuple<double, unsigned long, unsigned long>> thps_over_time, page_faults_over_time, sizes_over_time;
  unsigned long anon_size, thp_size, vs, rss;
  int soft_pf1, hard_pf1, soft_pf2, hard_pf2, soft_pf, hard_pf;

  const char* filename;
  if (new_filename != "") filename = new_filename;
  else filename = done_filename;

  setup_pagemaps(pid);

  printf("THP tracking process spawned! cpid %d waiting to run\n", cpid);
  while (stat (done_filename, &done_buffer) != 0 || stat (filename, &done_buffer) != 0) {
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

    get_sizes(stat_filename.c_str(), &vs, &rss);
    sizes_over_time.push_back(make_tuple(elapsed_seconds.count(), vs, rss));

    usleep(THP_TIME);
  }
  printf("Process %d finished running!\n", pid);
  fflush(stdout);

  ofstream thp_file(thp_filename);
  for (tuple<double, unsigned long, unsigned long> data : thps_over_time) {
    thp_file << to_string(get<0>(data)) + " sec: " + to_string(get<1>(data)) + " anon, " + to_string(get<2>(data)) + " THPs\n";
  }
  thp_file.close();

  ofstream pf_file(pf_filename);
  tuple<double, unsigned long, unsigned long> sizes;
  unsigned int idx = 0;
  for (tuple<double, unsigned long, unsigned long> data : page_faults_over_time) {
    sizes = sizes_over_time[idx];
    idx++;
    pf_file << to_string(get<0>(data)) + " sec: VM size = " + to_string(get<1>(sizes)) + "KB, " + to_string(get<2>(sizes)) + "KB, " + to_string(get<1>(data)) + " soft pf, " + to_string(get<2>(data)) + " hard pf\n";
  }
  pf_file.close();

  return;
}

void read_promotions_over_time(map<unsigned long, vector<pair<unsigned long, unsigned long>>>* promotions_over_time, const char* filename) {
  char* line = NULL;
  size_t len = 0;
  string region_name;
  unsigned long start, tmp_time, time, tmp_offset, offset, num_offsets = 0;
  int err;

  FILE* fp = fopen(filename, "r");
  if (fp == NULL) {
    cout << "Error reading " << filename << endl;
    exit(EXIT_FAILURE);
  }

  while ((getline(&line, &len, fp)) != -1) {
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
      if (num_offsets > 0) (*promotions_over_time)[time].push_back(make_pair(start, num_offsets*pmd_pagesize)); 
      num_offsets = 0;
      offset = -1;
    }
    time = tmp_time;

    tmp_offset = stoul(result[1], NULL, 0) * pmd_pagesize; // current offset
    if (tmp_offset == offset+pmd_pagesize || num_offsets == 0) { // continous range or first offset
      if (num_offsets == 0) start = tmp_offset;
      num_offsets++;
    } else { // end of continuous range
      (*promotions_over_time)[time].push_back(make_pair(start, num_offsets*pmd_pagesize)); // end of continuous range

      // information for next range
      start = tmp_offset;
      num_offsets = 1;
    }
    offset = tmp_offset;
  }
  (*promotions_over_time)[time].push_back(make_pair(start, num_offsets*pmd_pagesize));

  /*
  map<unsigned long, vector<pair<unsigned long, unsigned long>>>::iterator it;
  for (it = promotions_over_time->begin(); it != promotions_over_time->end(); it++) {
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
void launch_promoter(int cpid, const char* promotion_filename, const char* new_filename="") {
  struct stat done_buffer;
  unsigned long time, base, offset;
  struct iovec iov;
  int err;
  
  const char* filename;
  if (new_filename != "") filename = new_filename;
  else filename = done_filename;

  map<unsigned long, vector<pair<unsigned long, unsigned long>>> promotions_over_time;
  map<unsigned long, vector<pair<unsigned long, unsigned long>>>::iterator it;

  setup_pagemaps(pid);
  if (promotion_filename != "") read_promotions_over_time(&promotions_over_time, promotion_filename);

  for (it = promotions_over_time.begin(); it != promotions_over_time.end(); it++) {
    cout << "KEY: " << it->first << endl;
    fflush(stdout);
  }
  it = promotions_over_time.begin();

  printf("Promotion process spawned! cpid %d waiting to run\n", cpid);
  while (stat (done_filename, &done_buffer) != 0 || stat (filename, &done_buffer) != 0) {
    usleep(SLEEP_TIME);
  }
  printf("cpid %d running\n", cpid);
  fflush(stdout);

  usleep(APP_TIME);

  vector<pair<unsigned long, unsigned long>> madvise_ranges;
  while(0 == kill(pid, 0)) {
    time = it->first;
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
    it++;
    usleep(PROMOTION_SLEEP_TIME);
  }
  fflush(stdout);
  
  return;
}
