void init_sssp(int num_nodes, int start_seed, unsigned long *ret, unsigned long **in_wl, unsigned long *in_index, unsigned long **out_wl, unsigned long *out_index) {
  void *tmp = nullptr;

  posix_memalign(&tmp, 1 << 21, num_nodes * 5 * sizeof(unsigned long));
  *in_wl = static_cast<unsigned long*>(tmp);
  
  posix_memalign(&tmp, 1 << 21, num_nodes * 5 * sizeof(unsigned long));
  *out_wl = static_cast<unsigned long*>(tmp);
  
  for (unsigned long i = 0; i < num_nodes; i++) {
    ret[i] = WEIGHT_MAX;
  }

  *in_index = 0;
  *out_index = 0;

  for (unsigned long i = start_seed; i < start_seed+SEEDS; i++) {
    unsigned long index = *in_index;
    *in_index = index + 1;
    (*in_wl)[index] = i;
    ret[i] = 0;
  }
}

void print_regions(csr_graph G, unsigned long *ret, unsigned long *in_wl, unsigned long *out_wl) {
  vector<string> data_names{"NODE_ARRAY", "EDGE_ARRAY", "EDGE_VALUES", "PROP_ARRAY", "IN_WL", "OUT_WL"};
  vector<pair<uint64_t, uint64_t>> reuse_mem_regions;

  reuse_mem_regions.push_back(make_pair((uint64_t) G.node_array, (uint64_t)(G.node_array+G.nodes+1)));
  reuse_mem_regions.push_back(make_pair((uint64_t) G.edge_array, (uint64_t)(G.edge_array+G.edges)));
  reuse_mem_regions.push_back(make_pair((uint64_t) G.edge_values, (uint64_t)(G.edge_values+G.edges)));
  reuse_mem_regions.push_back(make_pair((uint64_t) ret, (uint64_t)(ret+G.nodes)));
  reuse_mem_regions.push_back(make_pair((uint64_t) in_wl, (uint64_t)(in_wl+G.nodes*5)));
  reuse_mem_regions.push_back(make_pair((uint64_t) out_wl, (uint64_t)(out_wl+G.nodes*5)));

  uint64_t start, end;
  cout << "\nMemory Regions:" << endl;
  for (unsigned int i = 0; i < reuse_mem_regions.size(); i++) {
    start = (uint64_t)(reuse_mem_regions[i].first/HUGE_PAGE_SIZE); // align to page
    end = (uint64_t)(reuse_mem_regions[i].second+HUGE_PAGE_SIZE-1)/HUGE_PAGE_SIZE;
    cout << data_names[i] << ": starting base = " << hex << start << ", ending base = " << end << endl;
  }
  cout << endl;
}

void sssp(csr_graph G, unsigned long *ret, unsigned long *in_wl, unsigned long* in_index, unsigned long *out_wl, unsigned long *out_index, int tid, int num_threads) {
  int hop = 1;

  while (*in_index > 0) {
    printf("-- epoch %d %lu --> push\n", hop, *in_index);
    for (unsigned long i = tid; i < *in_index; i += num_threads) {
      unsigned long node = in_wl[i];
      for (unsigned long e = G.node_array[node]; e < G.node_array[node+1]; e++) {
        unsigned long edge_index = G.edge_array[e];

        weightT curr_dist = ret[edge_index];
        weightT new_dist = ret[node] + G.edge_values[e];

        if (new_dist < curr_dist) {
          ret[edge_index] = new_dist;

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

/*
int main(int argc, char** argv) {

  string graph_fname;
  const char *size_filename = "num_nodes_edges.txt";
  csr_graph G;
  unsigned long num_nodes;
  int start_seed = 0;

  assert(argc >= 2);
  graph_fname = argv[1];
  if (argc >= 3) start_seed = atoi(argv[2]);

  ifstream nodes_edges_file(graph_fname + size_filename);
  nodes_edges_file >> num_nodes;

  unsigned long in_index, out_index, *in_wl, *out_wl, *ret;
  ret = (unsigned long*) malloc(num_nodes * sizeof(unsigned long));

  G = parse_bin_files(graph_fname, 0);
  init_bfs(G, start_seed, &in_index, &out_index, &in_wl, &out_wl, ret);
  kernel(G, ret, in_wl, &in_index, out_wl, &out_index, 0 , 1);

  return 0;
}
*/
