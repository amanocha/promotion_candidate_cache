#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <bits/stdc++.h>
#include "common.h"

using namespace std;

bool csr_sort(const tuple<int, int, int>& a,const tuple<int, int, int>& b) {
  return (a < b);
}

void _kernel_(csr_graph G, unsigned long * deg, float * bounds, unsigned long * indices, unsigned long * groups, unsigned long * ret, int tid, int num_threads) {
  for (unsigned long i = 0; i < G.nodes; i++) {
    deg[i] = G.node_array[i+1]-G.node_array[i];
  }
 
  float avg_deg = (float) G.edges/G.nodes;
  unsigned long idx, g_idx;

  for (unsigned long i = tid; i < G.nodes; i += num_threads) {
    idx = 0;
    unsigned long d = deg[i];

    while (idx < 8) {
      if (avg_deg*bounds[idx] <= d) {
        g_idx = indices[idx];
        groups[idx*G.nodes + g_idx] = i;
        indices[idx]++;
        break; 
      }
      idx++;
    } 
  }   

  idx = 0;
  for (int i = 0; i < 8; i++) {
    g_idx = G.nodes*i;
    while (groups[g_idx] != -1) {
      unsigned long node = groups[g_idx];
      ret[node] = idx;
      g_idx++;
      idx++;
    }   
  }
}

int main(int argc, char** argv) {

  char *graph_fname;
  csr_graph G;

  assert(argc >= 2);
  graph_fname = argv[1];
  
  //G = parse_csr_graph(graph_fname);
  G = parse_bin_files(graph_fname);

  cout << "INFO" << endl;
  cout << G.nodes << " " << G.edges << endl;
  cout << G.node_array[0] << " " << G.node_array[1] << " " << G.node_array[G.nodes-1] << " " << G.node_array[G.nodes] << endl;

  unsigned long * deg = (unsigned long *) malloc(sizeof(unsigned long) * G.nodes);
  unsigned long * ret = (unsigned long *) malloc(sizeof(unsigned long) * G.nodes);
  float bounds[8] = {32, 16, 8, 4, 2, 1, 0.5, 0};
  unsigned long indices[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  unsigned long * groups = (unsigned long *) malloc(sizeof(unsigned long) * 8 * G.nodes);
  
  for (unsigned long i = 0; i < G.nodes*8; i++) {
    groups[i] = -1;
  }
 
  printf("\n\nstarting kernel\n");
  auto start = chrono::system_clock::now();

  _kernel_(G, deg, bounds, indices, groups, ret, 0 , 1);

  printf("\nending kernel");
  auto end = std::chrono::system_clock::now();
  chrono::duration<double> elapsed_seconds = end-start;
  cout << "\nkernel computation time: " << elapsed_seconds.count() << "s\n";

  ofstream outfile("dbg.txt");
  vector<tuple<unsigned long, unsigned long, unsigned long>> sorted_graph;
  unsigned long num_edges = 0; 
  for (unsigned long n = 0; n < G.nodes; n++) {
    for (unsigned long e = G.node_array[n]; e < G.node_array[n+1]; e++) {
      unsigned long edge_index = G.edge_array[e];
      unsigned long edge_val = G.edge_values[e];
      //outfile << ret[n] << " " << ret[edge_index] << " " << edge_val << endl;
      sorted_graph.push_back(make_tuple(ret[n], ret[edge_index], edge_val));
      num_edges++;
      if (num_edges % 1000000 == 0) printf("%% %.2f written\n", float(num_edges)/float(G.edges)*100.0);
    }
  }
  sort(sorted_graph.begin(), sorted_graph.end(), csr_sort);
  outfile << "c " << G.edges << " " << G.nodes << endl;
  for (unsigned long i = 0; i < sorted_graph.size(); i++) {
    outfile << get<0>(sorted_graph[i]) << " " << get<1>(sorted_graph[i]) << " " << get<2>(sorted_graph[i]) << endl;
  }
  outfile.close();
  //cout << "c " << G.edges << " " << G.nodes << endl;

  free(deg);
  free(ret);
  free(groups);
  
  clean_csr_graph(G);

  return 0;
}
