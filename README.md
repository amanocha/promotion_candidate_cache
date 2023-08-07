# Architectural Support for Optimizing Huge Page Selection Within the OS

## Graph Applications

 1. **Breadth First Search (BFS)** - Given a starting (root) vertex, determine the minimum number of hops to all vertices. 

	In addition to its direct use in network analysis, e.g. LinkedIn degree separation, this algorithm also forms the basic building block of many other graph applications such as Graph Neural Networks, Connected Components, and Betweenness Centrality.
 2. **Single-Source Shortest Paths (SSSP)** - Given a starting (root) vertex, determine the minimum distance (sum of edge weights) to all vertices. 

	This algorithm is utilized in navigation and transportation problems as well as network utilization and its more general form is the $k$-shortest paths algorithm. 
 3. **PageRank (PR)** - Determine the "rank" or importance of all vertices (e.g. pages), where vertex scores are distributed to outgoing neighbors and updated until all scores converge, i.e. change by less than a threshold $\epsilon$. 

	Variants of this algorithm are used in ranking algorithms, e.g. of webpages, keywords, etc. 

The source codes for these applications are located in `launch/[APP NAME]`. These implementations are based on those from the [GraphIt framework](https://graphit-lang.org/).

## Datasets
We used the following datasets and their reordered variants:
1. **Kronecker_25** - synthetic power-law network
2. **Twitter** - real social network
3. **Sd1_Arc** - real web network

These datasets are stored in [Compressed Sparse Row (CSR) Format](https://en.wikipedia.org/wiki/Sparse_matrix) as binary files. Each dataset has the following files:

 - `num_nodes_edges.text` stores the number of vertices and edges in the network, which is used to determine the amount of data to dynamically allocate for the graph application before it is populated with values
 - `node_array.bin` stores values in the *vertex array*, which are the cumulative number of neighbors each vertex has
 - `edge_array.bin` stores values in the *edge array*, which are the neighbor IDs for each vertex (this array is indexed by the vertex array) 
 - `edge_values.bin` stores values in the *values array*, which are edge weights for the path to each vertex's neighbor, if such weights exist

We use the Kronecker network generator from the [GAP Benchmark Suite](http://gap.cs.berkeley.edu/benchmark.html) and the real-world networks are from [SuiteSparse](https://sparse.tamu.edu/) and [SNAP](http://snap.stanford.edu/).

All dataset files are available [here](https://decades.cs.princeton.edu/datasets/big/). They can be downloaded via `wget`. See instructions below ("Data Setup") on how to set up data. 

### Degree-Based Grouping (DBG)

We perform dataset preprocessing as a standalone, separate step and store the preprocessed datasets as binary files as well. The code to perform the preprocessing and generate the dataset files is available at `utils/dbg.cpp`. It takes in a dataset folder (storing the 4 files described above) and outputs a file `dbg.txt` storing the preprocessed dataset in edgelist format:

    cd utils
    make
    ./dbg [PATH_TO_DATASET_FOLDER]

`PATH_TO_DATASET_FOLDER` is the path to the original dataset to be preprocessed. The edgelist file `dbg.txt` then needs to be converted to a binary file. This can be achieved with the following commands:

    cd graph_conversion
    make
    mkdir [PREPROCESSED_DATASET_FOLDER]
    ./edgelist_to_binary ../dbg.txt [PREPROCESSED_DATASET_FOLDER]

`PREPROCESSED_DATASET_FOLDER` is the name of the preprocessed dataset where the 4 dataset files, `num_nodes_edges.text`, `node_array.bin`, `edge_array.bin`, and `edge_values.bin`, for the resulting preprocessed CSR will be created. Once the creation of this new dataset folder is complete, `dbg.txt` can be removed and the folder can be moved to the `data/` folder (after performing data setup) where all other datasets are stored.

For details on the DBG algorithm, see the reference below.

**Reference**
Priyank Faldu, Jeff Diamond, and Boris Grot. 2020. [A Closer Look at Lightweight Graph Reordering](https://faldupriyank.com/papers/DBG_IISWC19.pdf). In *2019 IEEE International Symposium on Workload Characterization (IISWC)*. Institute of Electrical and Electronics Engineers (IEEE), United States, 1–13. 

## Experiments

### Prerequisites
 - Bash
 - Python3
 - Linux v5.15
 - Linux Perf
 - numactl (and NUMA support)
 - Root access

### NUMA Effects

In order to avoid NUMA latency effects, e.g. a combination of local and remote access latencies interfere with experiments, our experimental methodology is designed such that all application memory is allocated and used from one NUMA node, while all data is stored on another. Therefore, one node must be reserved for data storage. We used node 0 and  recommend using a node such that the applications can run on another node that is not node 0. Another separate node must be reserved for application execution.

### Data Setup

All data must be stored in tmpfs to eliminate page cache effects. We have provided a script to automate the process of downloading the data and setting it up accordingly. First, the home directory (where this workflow code is located) needs to be configured. Edit line 3 in `setup.sh` to specify the full path of where this file is located:

    HOME_DIR=/home/aninda/promotion_candidate_cache # EDIT THIS VALUE (WORKFLOW DIRECTORY)

To configure the tmpfs data to be pinned on a NUMA node other than node 0, edit line 4 in `setup.sh`:

    NUMA_NODE=0 # EDIT THIS VALUE (NUMA NODE) 

Then execute `sudo bash setup.sh`.

### Characterizing Reuse Distance

You can configure TLB size by modifying line 4 in `run.sh`:

    TLB_SIZE=1024

Then execute `bash run.sh`. 
