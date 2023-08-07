float alpha = 0.85;
float epsilon = 0.01;

void init_pagerank(int run_kernel, csr_graph G, double **x, double **in_r, double **ret, unsigned long **in_wl, unsigned long *in_index, unsigned long **out_wl, unsigned long *out_index) {
  void *tmp = nullptr;

  posix_memalign(&tmp, 1 << 21, G.nodes * sizeof(unsigned long));
  *in_wl = static_cast<unsigned long*>(tmp);

  posix_memalign(&tmp, 1 << 21, G.nodes * sizeof(unsigned long));
  *out_wl = static_cast<unsigned long*>(tmp);

  posix_memalign(&tmp, 1 << 21, G.nodes * sizeof(double));
  *x = static_cast<double*>(tmp);

  posix_memalign(&tmp, 1 << 21, G.nodes * sizeof(double));
  *in_r = static_cast<double*>(tmp);

  for (unsigned long v = 0; v < G.nodes; v++) {
    (*x)[v] = 1 - alpha;
    (*in_r)[v] = 0;
    (*ret)[v] = 0;
  }

  *in_index = 0;
  *out_index = 0;

  for (unsigned long v = 0; v < G.nodes; v++) {
    unsigned long G_v = G.node_array[v+1]-G.node_array[v];
    for (unsigned long i = G.node_array[v]; i < G.node_array[v+1]; i++) {
      unsigned long w = G.edge_array[i];
      (*in_r)[w] = (*in_r)[w] + 1.0/G_v;
    }

    unsigned long index = *in_index;
    *in_index = index + 1;
    (*in_wl)[index] = v;
  }

  for (unsigned long v = 0; v < G.nodes; v++) {
    (*in_r)[v] = (1-alpha)*alpha*((*in_r)[v]);
  }
}

void print_regions(csr_graph G, double *x, double *in_r, double *ret, unsigned long *in_wl, unsigned long *out_wl) {
  vector<string> data_names{"NODE_ARRAY", "EDGE_ARRAY", "X_ARRAY", "IN_R_ARRAY", "PROP_ARRAY", "IN_WL", "OUT_WL"};
  vector<pair<uint64_t, uint64_t>> reuse_mem_regions;
  
  reuse_mem_regions.push_back(make_pair((uint64_t) G.node_array, (uint64_t)(G.node_array+G.nodes+1)));
  reuse_mem_regions.push_back(make_pair((uint64_t) G.edge_array, (uint64_t)(G.edge_array+G.edges)));
  reuse_mem_regions.push_back(make_pair((uint64_t) x, (uint64_t)(x+G.nodes)));
  reuse_mem_regions.push_back(make_pair((uint64_t) in_r, (uint64_t)(in_r+G.nodes)));
  reuse_mem_regions.push_back(make_pair((uint64_t) ret, (uint64_t)(ret+G.nodes)));
  reuse_mem_regions.push_back(make_pair((uint64_t) in_wl, (uint64_t)(in_wl+G.nodes)));
  reuse_mem_regions.push_back(make_pair((uint64_t) out_wl, (uint64_t)(out_wl+G.nodes)));

  uint64_t start, end;
  cout << "\nMemory Regions:" << endl;
  for (unsigned int i = 0; i < reuse_mem_regions.size(); i++) {
    start = (uint64_t)(reuse_mem_regions[i].first/HUGE_PAGE_SIZE); // align to page
    end = (uint64_t)(reuse_mem_regions[i].second+HUGE_PAGE_SIZE-1)/HUGE_PAGE_SIZE;
    cout << data_names[i] << ": starting base = " << hex << start << ", ending base = " << end << endl;
  }
  cout << endl;
}

float fetch_add_double(volatile double* addr, double to_add) {
  union {
    unsigned long ulong_val;
    double double_val;
  } value, ret;

  do {
    value.double_val = *addr;
    ret.double_val = value.double_val + to_add;
  } while (!atomic_compare_exchange_weak_explicit((atomic_ulong*) addr, &value.ulong_val, ret.ulong_val, memory_order_relaxed, memory_order_relaxed));

  return value.double_val;
}

void pagerank(csr_graph G, double *x, double *in_r, double *ret, unsigned long *in_wl, unsigned long *in_index, unsigned long *out_wl, unsigned long *out_index, int tid, int num_threads) {
  unsigned long v, G_v, in_r_v, w, start, end, index;
  double new_r, r_old;
  int hop = 1;

  while (*in_index > 0) {
    if (tid == 0) printf("-- epoch %d %lu\n", hop, *in_index);
    for (unsigned long i = tid; i < *in_index; i+=num_threads) {
      v = in_wl[i];
      x[v] = x[v] + in_r[v];

      start = G.node_array[v];
      end = G.node_array[v+1];
      G_v = end-start;
      new_r = in_r[v]*alpha/G_v;
      for (unsigned long e = start; e < end; e++) {
        w = G.edge_array[e];
        double r_old = fetch_add_double(&ret[w], new_r);
        if (ret[w] >= epsilon && r_old < epsilon) {
          index = (unsigned long) atomic_fetch_add((atomic_ulong*) out_index, 1);
          out_wl[index] = w;
        }
      }
      in_r[v] = 0;
    }

    unsigned long *tmp_wl = out_wl;
    out_wl = in_wl;
    in_wl = tmp_wl;
    hop++;

    double *tmp_r = ret;
    ret = in_r;
    in_r = tmp_r;
    
#pragma omp barrier

    if (tid == 0) {
      *in_index = *out_index;
      *out_index = 0;
    }

#pragma omp barrier
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
