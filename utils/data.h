int lock_memory(char* addr, size_t size) {
    cout << "Called lock_memory with " << (void*) addr << " and " << size << endl;
    unsigned long page_offset, page_size;
    page_size = sysconf(_SC_PAGE_SIZE);
    page_offset = (unsigned long) addr % page_size;

    addr -= page_offset;  /* Adjust addr to page boundary */
    size += page_offset;  /* Adjust size with page_offset */

    cout << "Locking " << size << "B of memory" << endl;
    return (mlock(addr, size));  /* Lock the memory */
}

int unlock_memory(char* addr, size_t size) {
    unsigned long page_offset, page_size;

    page_size = sysconf(_SC_PAGE_SIZE);
    page_offset = (unsigned long) addr % page_size;

    addr -= page_offset;  /* Adjust addr to page boundary */
    size += page_offset;  /* Adjust size with page_offset */

    return ( munlock(addr, size) );  /* Unlock the memory */
}

void pin_data(csr_graph G, unsigned long node) {
    unsigned long start, end, num_edges;
    int err;

    start = G.node_array[node];
    end = G.node_array[node+1];
    num_edges = end-start;

    err = lock_memory((char*) &start, num_edges * sizeof(unsigned long));
    if (err != 0) perror("Error with pinning edge data!");
}

void unpin_data(csr_graph G, unsigned long node) {
    unsigned long start, end, num_edges;
    int err;

    start = G.node_array[node];
    end = G.node_array[node+1];
    num_edges = end-start;

    err = unlock_memory((char*) &start, num_edges * sizeof(unsigned long));
    if (err != 0) perror("Error with unpinning edge data!");
}

int count_directory(const char* dir_name, const char* prefix) {
    DIR *d;
    struct dirent *dir;
    d = opendir(dir_name);
    int num = 0;
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            //printf("%s\n", dir->d_name);
            std::string name = dir->d_name;
            if (name.rfind(prefix, 0) == 0) num++;
        }
        closedir(d);
    }
    return num;
}