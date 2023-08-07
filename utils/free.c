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

static int print_buddyinfo(int node_input)
{
	char buf[4 * PAGE_SIZE] = {0};
	int ret, off, fd, i;
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
				for (i = 0; i < MAX_ORDER; i++)
					nr[i] = n[i];
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
	return 0;
}

int main(int argc, char **argv)
{
	char filename[1000];
	char *line, *ptr;
	FILE *fp;
	size_t len;
	unsigned long pfn;
	int ret, order, node, i;

	if (strcpy(filename, argv[1]) == NULL) {
		perror("Invalid filename");
		return EXIT_FAILURE;
	}
	order = atoi(argv[2]);
	node = atoi(argv[3]);
	line = NULL;
	
	fp = fopen(filename, "r");
	if (fp == NULL) {
		perror("Invalid file");
		exit(EXIT_FAILURE);
	}

	printf("Freeing pages: ");
	i = 0;
	while (getline(&line, &len, fp) != -1) {
		pfn = strtoul(line, &ptr, 10);
		ret = syscall(SYS_user_free_pages, pfn, 0);
		if (ret != 0) {
			printf("Invalid PFN to free: %lu\n", pfn);
			fflush(stdout);
		}

		if ((i*(PAGE_SIZE << order)) % SZ_GB == 0) {
			putchar('.');
			fflush(stdout);
		}
		i++;
	}
	fclose(fp);
		
	printf("\n\n");
	print_buddyinfo(node);

	return EXIT_SUCCESS;
}
