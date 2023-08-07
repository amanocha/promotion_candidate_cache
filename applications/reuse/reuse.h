#define TRACK_ACCESS 1

unsigned int REUSE_PAGE_SIZE;
double alpha = 0.9;
unsigned long total_num_access = 0;
unordered_map<int, uint64_t> access_num_map;
vector<string> data_names{"NODE_ARRAY", "EDGE_ARRAY", "PROP_ARRAY", "IN_WL", "OUT_WL"};
vector<pair<uint64_t, uint64_t>> reuse_mem_regions;
unordered_map<uint64_t, tuple<unsigned long, double, unsigned long, double, double, unsigned long, double>> reuse_map;

// Update list of memory regions
void update_mem_regions(csr_graph G, unsigned long *ret, unsigned long *in_wl, unsigned long *out_wl, unsigned long *in_idx, unsigned long *out_idx) {
  reuse_mem_regions.push_back(make_pair((uint64_t) G.node_array, (uint64_t)(G.node_array+G.nodes+1)));
  reuse_mem_regions.push_back(make_pair((uint64_t) G.edge_array, (uint64_t)(G.edge_array+G.edges)));
  reuse_mem_regions.push_back(make_pair((uint64_t) ret, (uint64_t)(ret+G.nodes)));
  reuse_mem_regions.push_back(make_pair((uint64_t) in_wl, (uint64_t)(in_wl+G.nodes*2)));
  reuse_mem_regions.push_back(make_pair((uint64_t) out_wl, (uint64_t)(out_wl+G.nodes*2)));

  /*cout << "Memory Regions:" << endl;
  for (int i = 0; i < reuse_mem_regions.size(); i++) {
    cout << i << ": start = " << hex << reuse_mem_regions[i].first << ", end = " << hex << reuse_mem_regions[i].second << endl;
    cout << "\t start = " << hex << (uint64_t) (reuse_mem_regions[i].first/REUSE_PAGE_SIZE) << ", end = " << hex << (uint64_t) ceil(reuse_mem_regions[i].second/REUSE_PAGE_SIZE) << endl;
  }*/
}

void track_access(uint64_t vaddr, int pc, bool is_load=true, bool print=false) {
    uint64_t base, reuse_dist, access_num, num_data_pts;
    double ema, sum_sq, std_dev, std_dev2;

    if (access_num_map.find(pc) == access_num_map.end()) {
      access_num_map[pc] = 0;
    }
    access_num = access_num_map[pc];

    base = (uint64_t) (vaddr/REUSE_PAGE_SIZE);

    if (reuse_map.find(base) == reuse_map.end()) { // cold L1 TLB miss won't have reuse tracked anyway
      reuse_map[base] = make_tuple(0, 0, 0, 0, 0, 0, 0);
    } else {
      reuse_dist = access_num - get<0>(reuse_map[base]);
      get<0>(reuse_map[base]) = access_num;

      sum_sq = get<1>(reuse_map[base]); // current sum of differences (reuse dist - EMA)
      num_data_pts = get<2>(reuse_map[base]); // num of reuse dists
      ema = get<3>(reuse_map[base]); // current mean (EMA)
      std_dev = sqrt(sum_sq/num_data_pts); // current std dev

      if (print) cout << reuse_dist << ": EMA = " << ema << ", std dev = " << std_dev << "; sum = " << sum_sq << ", n = " << num_data_pts << ", diff = " << (reuse_dist-ema) << endl;

      // decide whether to consider new data point
      if (num_data_pts < 2 || 3*std_dev >= abs(reuse_dist-ema) || std_dev > 1) {
        if (ema == 0) ema = reuse_dist;
        else ema = ema*alpha + reuse_dist*(1-alpha);
        sum_sq += pow((ema-reuse_dist), 2);
        num_data_pts++;

        get<1>(reuse_map[base]) = sum_sq;
        get<2>(reuse_map[base]) = num_data_pts;
        get<3>(reuse_map[base]) = ema;
      }

      // update true data
      sum_sq = get<4>(reuse_map[base]);
      num_data_pts = get<5>(reuse_map[base]);
      ema = get<6>(reuse_map[base]);

      if (ema == 0) ema = reuse_dist;
      else ema = ema*alpha + reuse_dist*(1-alpha);
      sum_sq += pow((ema-reuse_dist), 2);
      num_data_pts++;

      get<4>(reuse_map[base]) = sum_sq;
      get<5>(reuse_map[base]) = num_data_pts;
      get<6>(reuse_map[base]) = ema;

      //if (print) cout << hex << base << ", curr reuse dist = " << get<3>(reuse_map[base]) << endl;
    }

    access_num_map[pc]++;
}

void summarize() {
  unsigned long access_num, num_data_pts;
  double reuse_dist, ema, sum_sq, std_dev, std_dev2;

  cout << "\nMemory Regions:" << endl;
  for (int i = 0; i < reuse_mem_regions.size(); i++) {
    cout << data_names[i] << ": starting base = " << hex << (uint64_t) (reuse_mem_regions[i].first/REUSE_PAGE_SIZE) << ", ending base = " << hex << (uint64_t) (reuse_mem_regions[i].second+REUSE_PAGE_SIZE-1)/REUSE_PAGE_SIZE << endl;
    ema = 0;
    sum_sq = 0;
    num_data_pts = 0;
    for (uint64_t base = (uint64_t) (reuse_mem_regions[i].first/REUSE_PAGE_SIZE); base < (uint64_t) (reuse_mem_regions[i].second+REUSE_PAGE_SIZE-1)/REUSE_PAGE_SIZE; base++) {
      if (reuse_map.find(base) != reuse_map.end()) {
        reuse_dist = get<3>(reuse_map[base]);

        std_dev = sqrt(get<1>(reuse_map[base])/get<2>(reuse_map[base]));
        std_dev2 = sqrt(get<4>(reuse_map[base])/get<5>(reuse_map[base]));
        cout << "\tbase = " << hex << base << dec << ", reuse dist = " << reuse_dist << ", n = " << get<2>(reuse_map[base]) << ", std dev = " << std_dev << ", true std dev = " << std_dev2 << endl;

        std_dev = sqrt(sum_sq/num_data_pts); // current std dev
        //if (print) cout << reuse_dist << ": EMA = " << ema << ", std dev = " << std_dev << "; sum = " << sum_sq << ", n = " << num_data_pts << ", diff = " << (reuse_dist-ema) << endl;

        // decide whether to consider new data point
        if (num_data_pts < 2 || 3*std_dev >= abs(reuse_dist-ema)) {
          if (ema == 0) ema = reuse_dist;
          else ema = ema*alpha + reuse_dist*(1-alpha);

          sum_sq += pow((ema-reuse_dist), 2);
          num_data_pts++;
        }
      }
    }
    cout << "combined average reuse dist = " << ema << "\n" << endl;
  }
}
