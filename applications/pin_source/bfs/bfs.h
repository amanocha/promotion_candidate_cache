void init_bfs(unsigned long num_nodes, int start_seed, unsigned long *in_index, unsigned long *out_index, unsigned long **in_wl, unsigned long **out_wl, unsigned long *ret) {
  void *tmp = nullptr;
  
  posix_memalign(&tmp, 1 << 21, num_nodes * 2 * sizeof(unsigned long));
  *in_wl = static_cast<unsigned long*>(tmp);  
  
  posix_memalign(&tmp, 1 << 21, num_nodes * 2 * sizeof(unsigned long));
  *out_wl = static_cast<unsigned long*>(tmp);

  for (unsigned long i = 0; i < num_nodes; i++) {
    ret[i] = -1;
  }

  *in_index = 0;
  *out_index = 0;

  for (unsigned long i = start_seed; i < start_seed+SEEDS; i++) {
    unsigned long index = *in_index;
    *in_index = index + 1;
    *in_wl[index] = i;
    ret[i] = 0;
  }
}

void print_regions(csr_graph G, unsigned long *ret, unsigned long *in_wl, unsigned long *out_wl) {
  vector<string> data_names{"NODE_ARRAY", "EDGE_ARRAY", "PROP_ARRAY", "IN_WL", "OUT_WL"};
  vector<pair<uint64_t, uint64_t>> reuse_mem_regions;
  
  reuse_mem_regions.push_back(make_pair((uint64_t) G.node_array, (uint64_t)(G.node_array+G.nodes+1)));
  reuse_mem_regions.push_back(make_pair((uint64_t) G.edge_array, (uint64_t)(G.edge_array+G.edges)));
  reuse_mem_regions.push_back(make_pair((uint64_t) ret, (uint64_t)(ret+G.nodes)));
  reuse_mem_regions.push_back(make_pair((uint64_t) in_wl, (uint64_t)(in_wl+G.nodes*2)));
  reuse_mem_regions.push_back(make_pair((uint64_t) out_wl, (uint64_t)(out_wl+G.nodes*2)));

  uint64_t start, end;
  cout << "\nMemory Regions:" << endl;
  for (unsigned int i = 0; i < reuse_mem_regions.size(); i++) {
    start = (uint64_t)(reuse_mem_regions[i].first/HUGE_PAGE_SIZE); // align to page
    end = (uint64_t)(reuse_mem_regions[i].second+HUGE_PAGE_SIZE-1)/HUGE_PAGE_SIZE;
    cout << data_names[i] << ": starting base = " << hex << start << ", ending base = " << end << endl;
  }
  cout << endl;
}

void bfs(csr_graph G, unsigned long *ret, unsigned long *in_wl, unsigned long *in_index, unsigned long *out_wl, unsigned long *out_index, int tid, int num_threads) {

  int hop = 1;
  while (*in_index > 0) {
    printf("-- epoch %d %lu --> push\n", hop, *in_index);

    for (unsigned long i = tid; i < *in_index; i += num_threads) {
      unsigned long node = in_wl[i];

      for (unsigned long e = G.node_array[node]; e < G.node_array[node+1]; e++) {
	unsigned long edge_index = G.edge_array[e];
        unsigned long v = ret[edge_index];

        if (v == -1) {
          ret[edge_index] = hop;
          unsigned long index = *out_index;
          *out_index = *out_index + 1;
          out_wl[index] = edge_index;
        }
      }
    }

    unsigned long *tmp = out_wl;
    out_wl = in_wl;
    in_wl = tmp;
    hop++;
    *in_index = *out_index;
    *out_index = 0;
  }
}
