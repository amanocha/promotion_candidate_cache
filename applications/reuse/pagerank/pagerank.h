double alpha_pr = 0.85;
double epsilon = 0.01;

void init_pagerank(csr_graph G, double **x, double **in_r, double **ret, unsigned long **in_wl, unsigned long *in_index, unsigned long **out_wl, unsigned long *out_index) {
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
    (*x)[v] = 1 - alpha_pr;
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
    (*in_r)[v] = (1-alpha_pr)*alpha_pr*((*in_r)[v]);
  }
}

void pagerank(csr_graph G, double *x, double *in_r, double *ret, unsigned long *in_wl, unsigned long *in_index, unsigned long *out_wl, unsigned long *out_index, int tid, int num_threads) {
  unsigned long v, G_v, in_r_v, w, start, end;
  double new_r, r_old;
  int hop = 1;

  while (*in_index > 0) {
    printf("-- epoch %d %lu\n", hop, *in_index);
    for (unsigned long i = tid; i < *in_index; i+=num_threads) {
      v = in_wl[i];
      x[v] = x[v] + in_r[v];
      
      start = G.node_array[v];
      end = G.node_array[v+1];

#ifdef TRACK_ACCESS
      track_access((uint64_t) &in_wl[i], 0, true, false);
      track_access((uint64_t) &x[v], 0, true, false);
      track_access((uint64_t) &in_r[v], 0, true, false);
      track_access((uint64_t) &x[v], 0, true, false);
      track_access((uint64_t) &G.node_array[v], 0, true, false);
      track_access((uint64_t) &G.node_array[v+1], 0, true, false);
#endif

      G_v = end-start;
      new_r = in_r[v]*alpha_pr/G_v;
      for (unsigned long e = start; e < end; e++) {
        w = G.edge_array[e];
        r_old = ret[w];
        ret[w] = r_old + new_r;

#ifdef TRACK_ACCESS
        track_access((uint64_t) &G.edge_array[e], 0, true, false);
        track_access((uint64_t) &ret[w], 0, true, false);
        track_access((uint64_t) &ret[w], 0, true, false);
#endif

        if (ret[w] >= epsilon && r_old < epsilon) {
          unsigned long index = *out_index;
          *out_index = *out_index + 1;
          out_wl[index] = w;

#ifdef TRACK_ACCESS
          track_access((uint64_t) &out_wl[index], 0, true, false);
#endif
        }
      }

      in_r[v] = 0;

#ifdef TRACK_ACCESS
      track_access((uint64_t) &in_r[v], 0, true, false);
#endif
    }

    unsigned long *tmp_wl = out_wl;
    out_wl = in_wl;
    in_wl = tmp_wl;

    double *tmp_r = ret;
    ret = in_r;
    in_r = tmp_r;
    
    *in_index = *out_index;
    *out_index = 0;

    hop++;
  }
}
