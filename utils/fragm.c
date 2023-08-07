#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <sys/syscall.h>

#define MAX_ORDER	11
#define PAGE_SIZE	4096
#define SZ_GB		(1UL << 30)
#define SZ_MB 		1048576
#define MAX_LINE_LENGTH	500
#define PAGE_SHIFT	12
#define PAGEMAP_LENGTH	8

#define ENOMEM		12
#define EFAULT		14
#define BUFFER		0

#define SYS_user_alloc_pages 449
#define SYS_user_alloc_pages_node 450
#define SYS_user_free_pages 451

#define max(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b;       \
})

#define min(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})

int node_input = -1;
int order;
float percent;

static unsigned long get_numa_mem(const char* field) {
	unsigned long mem = 0;
	const char *command = "numastat -m";
	FILE *pipe;
	char pattern[100];
	char buffer[MAX_LINE_LENGTH];
	float val1, val2, val3, val;

	pipe = popen(command, "r");
	if (!pipe) {
		printf("popen failed!\n");
		exit(EXIT_FAILURE);
	}

	strcpy(pattern, field);
	strcat(pattern, "%f %f %f");
	while (!feof(pipe)) {
		if (fgets(buffer, 128, pipe) != NULL) {
			if (!strncmp(buffer, field, strlen(field))) break;
		}
	}

	if (sscanf(buffer, pattern, &val1, &val2, &val3) != 3) {
		printf("Could not match pattern!\n");
		exit(EXIT_FAILURE);
	}

	if (node_input == 0) val = val1;
	else if (node_input == 1) val = val2;
	else val = val3;

	mem = (unsigned long) val*SZ_MB;
	return mem;
}

static int print_buddyinfo(void)
{
	char buf[4 * PAGE_SIZE] = {0};
	int ret, off, fd, i, num_frags;
	ssize_t len;
	unsigned long nr[MAX_ORDER] = {0};
	unsigned long total = 0, cumulative = 0;

	fd = open("/proc/buddyinfo", O_RDONLY);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	len = read(fd, buf, sizeof(buf));
	if (len <= 0) {
		perror("read");
		close(fd);
		return -1;
	}

	off = 0;
	while (off < len) {
		int node;
		char __node[64], __zone[64], zone[64];
		unsigned long n[MAX_ORDER];
		int parsed;

		ret = sscanf(buf + off, "%s %d, %s %s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n%n",
			     __node, &node, __zone, zone, &n[0], &n[1], &n[2], &n[3], &n[4], &n[5], &n[6],
			     &n[7], &n[8], &n[9], &n[10], &parsed);
		//printf("%d %s %lu %lu %lu\n", node, zone, n[0], n[1], n[10]);
		if (ret < 15)
			break;

		off += parsed;

		if (node_input != -1) {
			if (node == node_input) {
				for (i = 0; i < MAX_ORDER; i++) {
					nr[i] = n[i];
					if (i >= order) {
						num_frags += nr[i]*pow(2, i-order);
					}
				}
			}
		} else {
			for (i = 0; i < MAX_ORDER; i++)
				nr[i] += n[i];
		}
	}

	for (i = 0; i < MAX_ORDER; i++)
		total += (PAGE_SIZE << i) * nr[i];

	printf("%-4s%10s%10s%10s%10s\n", "Order", "Pages", "Total", "%Free", "%Higher");
	for (i = 0; i < MAX_ORDER; i++) {
		unsigned long bytes = (PAGE_SIZE << i) * nr[i];

		cumulative += bytes;

		printf("%-4d %10lu %7.2lfGB %8.1lf%% %8.1lf%%\n", i, nr[i],
		       (double) bytes / SZ_GB,
		       (double) bytes / total * 100,
		       (double) (total - cumulative) / total * 100);
	}

	close(fd);
	printf("%d fragments available\n", num_frags);
	return num_frags;
}

static unsigned long get_pfn(unsigned long vaddr) {
	unsigned long pid, offset, paddr;
	FILE *pagemap;
	char filename[1024] = {0};
	int ret = -1;

	pid = getpid();
	sprintf(filename, "/proc/%ld/pagemap", pid);
	pagemap = fopen(filename, "rb");
	if (!pagemap) {
		perror("can't open file. ");
		goto err;
	}

	pread(fileno(pagemap), &paddr, sizeof(paddr), ((long)vaddr >> PAGE_SHIFT)*sizeof(paddr));

	paddr = paddr & 0x7fffffffffffff;

	/*offset = (vaddr / PAGE_SIZE) * PAGEMAP_LENGTH;
	if (fseek(pagemap, (unsigned long)offset, SEEK_SET) != 0) {
		perror("fseek failed. ");
		goto err;
	}

	if (fread(&paddr, 1, (PAGEMAP_LENGTH-1), pagemap) < (PAGEMAP_LENGTH-1)) {
		perror("fread fails. ");
		goto err;
	}

	paddr = paddr & 0x7fffffffffffff;
	//offset = vaddr % PAGE_SIZE;
	//paddr = (unsigned long)((unsigned long)paddr << PAGE_SHIFT) + offset;
	*/

	fclose(pagemap);
	return paddr;

err:
	fclose(pagemap);
	return ret;
}

static void sort(unsigned long *array, unsigned long length) {
	unsigned long i, j, tmp;
	for(i = 0; i < length; i++) {
		for(j = i + 1; j < length; j++) {
			if(array[j] < array[i]) {
				tmp = array[i];
				array[i] = array[j];
				array[j] = tmp;
			}
		}
	}
}

static int fragment_memory(int dentries)
{
	size_t size, mmap_size, i, j, region_size;
	char *area;
	int ret, iter = 0;
	unsigned long total_ram, free_ram, num_frags = 0, buddy_frags;
	unsigned long *data, *pgs;
	unsigned long pfn;

	buddy_frags = print_buddyinfo()-BUFFER;

	total_ram = get_numa_mem("MemTotal");
	free_ram = get_numa_mem("MemFree");
	region_size = min(free_ram, total_ram - ((PAGE_SIZE << order) - PAGE_SIZE));
	printf("total mem: %.1lf GB, free mem: %.1lf GB\n\n", (double) total_ram / SZ_GB, (double) free_ram / SZ_GB);
	
	/*
	mmap_size = total_ram;

	area = mmap(NULL, mmap_size, PROT_WRITE | PROT_READ,
		    MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	if (area == MAP_FAILED) {
		perror("mmap");
		return -1;
	}
	ret = madvise(area, mmap_size, MADV_HUGEPAGE);
	if (ret != 0) {
		perror("madvise");
		return -1;
	}

	size = PAGE_SIZE*pow(2, 0); //(PAGE_SIZE << order) - PAGE_SIZE;
	
	free_ram = get_numa_mem("MemFree")-512*SZ_MB;
	printf("%.1lf free GB; ", (double)free_ram / SZ_GB);

	printf("populating data in region of size %.1f GB: ", (double)region_size / SZ_GB);

	// Populate region
	for (i = 0; i < region_size; i += PAGE_SIZE) { 
		area[i] = 'f';
		pfn = get_pfn((unsigned long) &area[i]);
		if (i % SZ_GB == 0) {
			putchar('.');
			fflush(stdout);
		}
	}

	free_ram = get_numa_mem("MemFree");
	printf("\n%.1lf free GB; making holes of size %lu B: ", (double)free_ram / SZ_GB, size);
	
	// Make holes
	min = 1 << 31, max = 0;
	data = (unsigned long *) malloc((region_size/(PAGE_SIZE << order))*sizeof(unsigned long));
	for (i = 0; i < region_size*percent/100.0; i += (PAGE_SIZE << order)) {
		//ret = madvise(area + i, size, MADV_DONTNEED);
		pfn = get_pfn((unsigned long) &area[i]);
		data[i/(PAGE_SIZE << order)] = pfn;
		//if (i > 0) printf("data[%lu] = %lu, %d\n", i, data[i/(PAGE_SIZE << order)], data[i/(PAGE_SIZE << order)]-data[i/(PAGE_SIZE << order)-1]);
		if (min > pfn) min = pfn;
		if (max < pfn) max = pfn;
		ret = munmap(area + i, size);
		if (ret) {
			perror("hole creation error");
			return -1;
		}
		if (i % SZ_GB == 0) {
			putchar('.');
			fflush(stdout);
		}
		num_frags++;
	}
	printf("\n");
	sort(data, num_frags);
	printf("min PFN = %lu, max PFN = %lu\n", min, max);
		
	for (i = 0; i < num_frags; i++) {
		if (i > 0) printf("data[%lu] = %lu, %d\n", i, data[i], data[i]-data[i-1]);
	}*/
	
	num_frags = min((unsigned long) (region_size*percent/100.0)/(PAGE_SIZE << order), buddy_frags);
	free_ram = get_numa_mem("MemFree");
	printf("filling %lu holes (%.1lfGB): ", num_frags, (double)num_frags*PAGE_SIZE / SZ_GB);
	
	pgs = (unsigned long *) malloc(num_frags*sizeof(unsigned long));
	for (i = 0; i < num_frags; i++) {
		if (node_input != -1) pgs[i] = (unsigned long) syscall(SYS_user_alloc_pages_node, node_input, order);
		else pgs[i] = (unsigned long) syscall(SYS_user_alloc_pages, order);

		if ((pgs[i]) == -ENOMEM) {
			for (j = 0; j < i; j++) {
				ret = syscall(SYS_user_free_pages, pgs[j], order);
				if (ret != 0) {
					printf("Invalid PFN to free: %lu\n", pgs[j]);
					fflush(stdout);
				}
			}
			perror("Not enough memory");
			return -1;
		}

		for (j = 1; j < pow(2, order); j++) {
			ret = syscall(SYS_user_free_pages, pgs[i]+j, 0);
			if (ret != 0) { 
				printf("Invalid PFN to free: %lu\n", pgs[i]+j);
				fflush(stdout);
			}
		}
			
		if ((i*(PAGE_SIZE << order)) % SZ_GB == 0) {
			putchar('.');
			fflush(stdout);
		}
	}
		
	print_buddyinfo();
	free_ram = get_numa_mem("MemFree");
	printf("\n%.1lf free GB\n", (double)free_ram / SZ_GB);

	char buf[100];
	FILE *fp = fopen("done.txt", "w+");
	for (i = 0; i < num_frags; i++) {
		ret = sprintf(buf, "%lu", pgs[i]);
		fprintf(fp, buf);
		fprintf(fp, "\n");
	}
	fclose(fp);
	
	printf("Waiting...\n");	
	while (1) {};

 	/*
	printf("\nEnter character: ");
	char c = getchar();
	printf("Character entered: ");
	putchar(c);
	printf("\n");
	*/

	printf("\nFreeing pages: ");
	for (i = 0; i < num_frags; i++) {
		ret = syscall(SYS_user_free_pages, pgs[i], 0);
		
		if (ret != 0) {
			printf("Invalid PFN to free: %lu\n", pgs[i]);
			fflush(stdout);
		}

		if ((i*(PAGE_SIZE << order)) % SZ_GB == 0) {
			putchar('.');
			fflush(stdout);
		}
	}

	printf("\n\n");
	print_buddyinfo();

	return 0;
}

int main(int argc, char **argv)
{
	int dentries = 0, i, num_args = 5;

	if (argc < 2)
		goto bad_args;

	if (strcmp(argv[1], "stat") == 0) {
		print_buddyinfo();
		return EXIT_SUCCESS;
	} else if (strcmp(argv[1], "fragment") == 0) {
		if (strcmp(argv[2], "--dentries") == 0) {
			dentries = 1;
			num_args++;
		}

		if (argc == num_args) node_input = atoi(argv[argc - 3]);
		printf("NUMA node: %d\n\n", node_input);

		order = atoi(argv[argc - 2]);
		if (order < 1 || order > 18) {
			fprintf(stderr, "Order must be in [1; 18] range\n");
			return EXIT_FAILURE;
		}

		percent = atof(argv[argc - 1]);

		fragment_memory(dentries);
	} else {
		goto bad_args;
	}


	return EXIT_SUCCESS;

bad_args:
	fprintf(stderr,
		"Usage:\n"
		"    Fragment memory: fragment [--dentries] <order>\n"
		"    Show stats:      stat\n");
	return EXIT_FAILURE;
}
