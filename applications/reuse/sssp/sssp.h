void init_sssp(unsigned long num_nodes, int start_seed, unsigned long *in_index, unsigned long *out_index, unsigned long **in_wl, unsigned long **out_wl, unsigned long *ret) {
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

void sssp(csr_graph G, unsigned long *ret, unsigned long *in_wl, unsigned long* in_index, unsigned long *out_wl, unsigned long *out_index, int tid, int num_threads) {

  int hop = 1;
  while (*in_index > 0) {
    printf("-- epoch %d %lu\n", hop, *in_index);
    for (unsigned long i = tid; i < *in_index; i += num_threads) {
      unsigned long node = in_wl[i];

#ifdef TRACK_ACCESS
      track_access((uint64_t) &in_wl[i], 0, true, false);
      track_access((uint64_t) &G.node_array[node], 0, true, false);
      track_access((uint64_t) &G.node_array[node+1], 0, true, false);
#endif

      for (unsigned long e = G.node_array[node]; e < G.node_array[node+1]; e++) {
        unsigned long edge_index = G.edge_array[e];
        weightT curr_dist = ret[edge_index];
        weightT new_dist = ret[node] + G.edge_values[e];

#ifdef TRACK_ACCESS
        track_access((uint64_t) &G.edge_array[e], 0, true, false);
        track_access((uint64_t) &ret[edge_index], 0, true, false);
        track_access((uint64_t) &ret[node], 0, true, false);
        track_access((uint64_t) &G.edge_values[e], 0, true, false);
#endif
        if (new_dist < curr_dist) {
          ret[edge_index] = new_dist;

          unsigned long index = *out_index;
          *out_index = *out_index + 1;
          out_wl[index] = edge_index;
          
#ifdef TRACK_ACCESS
          track_access((uint64_t) &ret[edge_index], 0, false, false);
          track_access((uint64_t) &out_wl[index], 0, true, false);
#endif
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
