#pragma once

#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <climits>

#define SEEDS 1

using namespace std;

typedef unsigned long weightT;
//#define weight_max 1073741823

// CSR graph
class csr_graph {
public:
  unsigned long nodes;
  unsigned long edges;
  unsigned long *node_array;
  unsigned long *edge_array;
  weightT *edge_values;
};

class csc_graph {
public:
  unsigned long nodes;
  unsigned long edges;
  unsigned long *node_array;
  unsigned long *edge_array;
  weightT *edge_values;
};

// Parsing a bipartite graph
csr_graph parse_csr_graph(char *fname) {
  csr_graph ret;
  fstream reader(fname);
  string line;
  char comment;
  unsigned long first, second;
  float weight;

  auto start = chrono::system_clock::now();
  reader >> comment >> first >> second;
  cout << "graph: " << fname << "\nedges: " << first << "\nnodes: " << second << "\n\n";

  ret.nodes = second;
  ret.edges = first;
  ret.node_array = (unsigned long*) malloc(sizeof(unsigned long) * (ret.nodes + 1));
  ret.edge_array = (unsigned long*) malloc(sizeof(unsigned long) * (ret.edges));
  ret.edge_values = (weightT*) malloc(sizeof(weightT) * (ret.edges));

  unsigned long node = 0;
  ret.node_array[0] = 0;
  for(unsigned long i = 0; i < ret.edges; i++ ) {
    if (i % 100000 == 0) {
      printf("reading %% %.2f finished\r", (float(i)/float(ret.edges)) * 100);
      fflush(stdout);
    }
    reader >> first >> second >> weight;
    if (first != node) {
      while (node != first) {
	      node++;
	      ret.node_array[node] = i;
      }
    }
    ret.edge_array[i] = second;
    ret.edge_values[i] = (weightT) weight;
  }

  while (node != (ret.nodes-1)) {
    node++;
    ret.node_array[node] = ret.edges;
  }

  printf("reading %% 100.00 finished\n");
  ret.node_array[ret.nodes] = ret.edges;

  auto end = std::chrono::system_clock::now();
  chrono::duration<double> elapsed_seconds = end-start;
  cout << "Reading graph elapsed time: " << elapsed_seconds.count() << "s\n";

  return ret;
}

csc_graph convert_csr_to_csc(csr_graph G) {
  csc_graph ret;
  auto start = chrono::system_clock::now();

  printf("converting csr to csc\n");

  ret.nodes = G.nodes;
  ret.edges = G.edges;
  ret.node_array = (unsigned long*) malloc(sizeof(unsigned long) * (ret.nodes + 1));
  ret.edge_array = (unsigned long*) malloc(sizeof(unsigned long) * (ret.edges));
  unsigned long * incoming_edges = (unsigned long*) malloc(sizeof(unsigned long) * (ret.nodes));

  for (unsigned long i = 0; i < ret.nodes; i++) {
    incoming_edges[i] = 0;
  }
  for (unsigned long i = 0; i < ret.edges; i++) {
    ret.edge_array[i] = -1;
  }

  // count incoming edges
  unsigned long total_edges = 0;
  unsigned long duplicates = 0;
  for (unsigned long n = 0; n < ret.nodes; n++) {
    const unsigned long node = n;
    const unsigned long start = G.node_array[node];
    const unsigned long end = G.node_array[node+1];
    if (start > end) {
      cout << "start: " << start << endl;
      cout << "end: " << end << endl;
      cout << "node: " << n << endl;
      assert(0);
    }
    int previous = -1;
    for (unsigned long e = start; e < end; e++) {
      if (e > G.edges) {
	      assert(0);
      }
      const unsigned long edge_index = G.edge_array[e];
      if (edge_index >= ret.nodes) {
	      cout << edge_index << " " << ret.nodes << endl;
	      assert(0);
      }
      assert(edge_index < ret.nodes);
      incoming_edges[edge_index]++;
      total_edges++;

      if (edge_index == previous) {
	      duplicates++;
      }
      //cout << e << " " << edge_index << " " << previous << endl;
      //assert((int)edge_index >= previous);
      previous = edge_index;
    }
  }

  cout << "duplicates: " << duplicates << endl;

  // prefix sum
  for (unsigned long n = 1; n < ret.nodes; n++) {
    incoming_edges[n] += incoming_edges[n-1];
  }
  cout << incoming_edges[ret.nodes-1] << " " << ret.edges << endl;
  assert(incoming_edges[ret.nodes-1] == ret.edges);

  // finish up the return array
  ret.node_array[0] = 0;
  for (unsigned long n = 1; n <= ret.nodes; n++) {
    ret.node_array[n] = incoming_edges[n-1];
  }

  // repurpose incoming edges as an offset in the edge array
  for (unsigned long i = 0; i < ret.nodes; i++) {
    incoming_edges[i] = 0;
  }

  for (unsigned long n = 0; n < ret.nodes; n++) {
    const unsigned long node = n;
    const unsigned long start = G.node_array[node];
    const unsigned long end = G.node_array[node+1];
    for (unsigned long e = start; e < end; e++) {
      const unsigned long edge_index = G.edge_array[e];
      unsigned long base_index = ret.node_array[edge_index];
      unsigned long base_offset = incoming_edges[edge_index];
      ret.edge_array[base_index + base_offset] = n;
      incoming_edges[edge_index]++;
    }
  }

  for (unsigned long e = 0; e < ret.edges; e++) {
    assert(ret.edge_array[e] != -1);
  }

  free(incoming_edges);

  auto end = std::chrono::system_clock::now();
  chrono::duration<double> elapsed_seconds = end-start;
  cout << "Converting graph elapsed time: " << elapsed_seconds.count() << "s\n";
  return ret;
}

void clean_csr_graph(csr_graph in) {
  free(in.node_array);
  free(in.edge_array);
}

void clean_csc_graph(csc_graph in) {
  free(in.node_array);
  free(in.edge_array);
}
