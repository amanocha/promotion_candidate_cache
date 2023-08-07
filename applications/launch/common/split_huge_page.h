// SPDX-License-Identifier: GPL-2.0
/*
 * A test of splitting PMD THPs and PTE-mapped THPs from a specified virtual
 * address range in a process via <debugfs>/split_huge_pages interface.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <cctype>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/mount.h>
#include <malloc.h>
#include <stdbool.h>
#include <boost/algorithm/string.hpp>
#include <bits/stdc++.h>
#include <numa.h>
#include <numaif.h>

uint64_t pagesize;
unsigned int pageshift;
uint64_t pmd_pagesize;

const char *pagemap_template = "/proc/%d/pagemap";
const char *kpageflags_proc = "/proc/kpageflags";
char pagemap_proc[255];
int pagemap_fd;
int kpageflags_fd;

#define SYS_pidfd_open 434
#define SYS_process_madvise 440
#define SYS_user_kmalloc 449
#define SYS_user_kfree 450

#define MADV_IRREG 24
#define MADV_PROMOTE 25
#define MADV_PROMOTE_REF 26
#define MADV_DEMOTE 27

#define KB_SIZE 1024
#define PMD_SIZE_PATH "/sys/kernel/mm/transparent_hugepage/hpage_pmd_size"
#define SPLIT_DEBUGFS "/sys/kernel/debug/split_huge_pages"
#define PAGE_IDLE "/sys/kernel/mm/page_idle/bitmap"
#define SMAP_PATH "/proc/self/smaps"
#define INPUT_MAX 80

#define PID_FMT "%d,0x%lx,0x%lx"
#define HUGE_PFN_MASK ((1UL<<55)-1)
#define PFN_MASK     ((1UL<<55)-1)
#define PG_SWAPPED (1UL<<62)
#define PG_PRESENT (1UL<<63)

#define KPF_SWAPBACKED (1UL<<14)
#define KPF_THP      (1UL<<22)
#define KPF_IDLE (1UL<<25)

#define PFN_TO_IPF_IDX(pfn) pfn >> 6 << 3
#define BIT_AT(val, x)	(((val) & (1ull << x)) != 0)
#define SET_BIT(val, x) ((val) | (1ull << x))

using namespace std;

vector<string> split(const string &s, char delim) {
	stringstream ss(s);
	string item;
	vector<string> tokens;
	while (getline(ss, item, delim)) {
		tokens.push_back(item);
	}
	return tokens;
}

int is_backed_by_thp(char *vaddr) {
	uint64_t paddr;
	uint64_t page_flags;
	int pagemap_file = pagemap_fd;
	int kpageflags_file = kpageflags_fd;

	if (pagemap_file) {
		pread(pagemap_file, &paddr, sizeof(paddr),
			((long)vaddr >> pageshift) * sizeof(paddr));
		if (kpageflags_file) {
			pread(kpageflags_file, &page_flags, sizeof(page_flags),
				(paddr & PFN_MASK) * sizeof(page_flags));

			return !!(page_flags & KPF_THP);
		}
	}
	return -1;
}

int is_swap(char *vaddr) {
	uint64_t paddr;
	uint64_t page_flags;

	if (pagemap_fd) {
		pread(pagemap_fd, &paddr, sizeof(paddr), ((long)vaddr >> pageshift) * sizeof(paddr));
		if (kpageflags_fd) {
			pread(kpageflags_fd, &page_flags, sizeof(page_flags), (paddr & PFN_MASK) * sizeof(page_flags));
			cout << "is idle? " << !!(page_flags & KPF_IDLE) << ", is present? " << !!(paddr & PG_PRESENT) << endl;
			return !!(paddr & PG_SWAPPED);
		}
	}
	return -1;
}

int is_idle(char *vaddr) {
	int fd;
	uint64_t paddr;
	uint64_t pfn;
	uint64_t page_flags;
        uint64_t bitmap_data;
	size_t out;

	fd = open(PAGE_IDLE, O_RDWR);
	if (fd == -1) {
		perror("Open bitmap file failed");
		close(fd);
		exit(EXIT_FAILURE);
	}

	if (pagemap_fd) {
		pread(pagemap_fd, &paddr, sizeof(paddr), ((long)vaddr >> pageshift) * sizeof(paddr));
	} else {
		perror("Open pagemap file failed");
		close(fd);
		exit(EXIT_FAILURE);
		return -1;
	}

	if (is_backed_by_thp(vaddr)) pfn = paddr & HUGE_PFN_MASK;
	else pfn = paddr & PFN_MASK;

	out = pread(fd, &bitmap_data, sizeof(bitmap_data), PFN_TO_IPF_IDX(pfn));
	if (out == sizeof(bitmap_data)) {
		bitmap_data = (int) BIT_AT(bitmap_data, pfn % 64);
		close(fd);
		return !!(bitmap_data);
	}

	perror("is_idle failed");
	close(fd);
	exit(EXIT_FAILURE);
	return -1;
}

int set_idle(char* vaddr_start, char* vaddr_end) {
	int fd;
	uint64_t paddr;
	uint64_t pfn;
	uint64_t page_flags;
        uint64_t bitmap_data;
	size_t out;

	cout << "SET IDLE: " << (unsigned long) vaddr_start << " to " << (unsigned long) vaddr_end << endl;
	fflush(stdout);

	fd = open(PAGE_IDLE, O_RDWR);
	if (fd == -1) {
		perror("Open bitmap file failed");
		close(fd);
		exit(EXIT_FAILURE);
	}

	if (pagemap_fd) {
		for (uint64_t addr = (uint64_t) vaddr_start; addr < (uint64_t) vaddr_end; addr += pagesize) {
			pread(pagemap_fd, &paddr, sizeof(paddr), ((long) addr >> pageshift) * sizeof(paddr));
			pfn = paddr & PFN_MASK;
			out = pread(fd, &bitmap_data, sizeof(bitmap_data), PFN_TO_IPF_IDX(pfn));
			if (out == sizeof(bitmap_data)) {
				bitmap_data = SET_BIT(bitmap_data, pfn % 64);
				if (pwrite(fd, &bitmap_data, sizeof(bitmap_data), PFN_TO_IPF_IDX(pfn)) != sizeof(bitmap_data)) {
					perror("pwrite failed");
					close(fd);
					exit(EXIT_FAILURE);
					return -1;
				}
			} else {
				perror("pread failed");
				close(fd);
				exit(EXIT_FAILURE);
				return -1;
			}
		}
		close(fd);
		fflush(stdout);
		return 0;
	}

	close(fd);
	return -1;
}

static uint64_t read_pmd_pagesize(void) {
	int fd;
	char buf[20];
	ssize_t num_read;

	fd = open(PMD_SIZE_PATH, O_RDONLY);
	if (fd == -1) {
		perror("Open hpage_pmd_size failed");
		exit(EXIT_FAILURE);
	}
	num_read = read(fd, buf, 19);
	if (num_read < 1) {
		close(fd);
		perror("Read hpage_pmd_size failed");
		exit(EXIT_FAILURE);
	}
	buf[num_read] = '\0';
	close(fd);

	return strtoul(buf, NULL, 10);
}

static int write_file(const char *path, const char *buf, size_t buflen)
{
	int fd;
	ssize_t numwritten;

	fd = open(path, O_WRONLY);
	if (fd == -1)
		return 0;

	numwritten = write(fd, buf, buflen - 1);
	close(fd);
	if (numwritten < 1)
		return 0;

	return (unsigned int) numwritten;
}

static void write_debugfs(const char *fmt, ...) {
	char input[INPUT_MAX];
	int ret;
	va_list argp;

	va_start(argp, fmt);
	ret = vsnprintf(input, INPUT_MAX, fmt, argp);
	va_end(argp);

	if (ret >= INPUT_MAX) {
		printf("%s: Debugfs input is too long\n", __func__);
		exit(EXIT_FAILURE);
	}

	if (!write_file(SPLIT_DEBUGFS, input, ret + 1)) {
		perror(SPLIT_DEBUGFS);
		exit(EXIT_FAILURE);
	}
}

#define MAX_LINE_LENGTH 500

static bool check_for_pattern_addr(FILE *fp, const char *pattern, char *buf) {
	size_t pos1, pos2;
	uint64_t addr = strtoul(pattern, NULL, 16), range_start, range_end;
	string buf_str, buf_substr;
	while (fgets(buf, MAX_LINE_LENGTH, fp) != NULL) {
		buf_str = buf;
		pos1 = buf_str.find("-");
		if (pos1 != -1) {
			buf_substr = buf_str.substr(0, pos1);
			range_start = strtoul(buf_substr.c_str(), NULL, 16);

			pos2 = buf_str.find(" ");
			buf_substr = buf_str.substr(pos1+1, pos2);
			range_end = strtoul(buf_substr.c_str(), NULL, 16);
			if (range_start <= addr && range_end > addr) {
				return true;
			}
		}
	}
	return false;
}

static bool check_for_pattern(FILE *fp, const char *pattern, char *buf) {
	while (fgets(buf, MAX_LINE_LENGTH, fp) != NULL) {
		if (!strncmp(buf, pattern, strlen(pattern)))
			return true;
	}
	return false;
}

static tuple<unsigned long, unsigned long> read_smap_data(const char* smap_path=SMAP_PATH) {
	ifstream smap_file(smap_path);
	string line;
	vector<string> data;
	unsigned long thp_sum, anon_sum;

	thp_sum = 0, anon_sum = 0;
	while (getline(smap_file, line)) {
		if (line.find("AnonHugePages") != string::npos) {
			data = split(line, ' ');
			thp_sum += stoul(data[data.size()-2], nullptr, 0); 
		} else if (line.find("Anonymous") != string::npos) {
			data = split(line, ' ');
			anon_sum += stoul(data[data.size()-2], nullptr, 0); 
		}
	}
	
	return make_tuple(thp_sum, anon_sum);
}

static uint64_t check_huge(void *addr, const char* smap_path=SMAP_PATH, string field="AnonHugePages:") {
	uint64_t thp = 0;
	int ret;
	FILE *fp;
	char buffer[MAX_LINE_LENGTH];
	char addr_pattern[MAX_LINE_LENGTH];

	ret = snprintf(addr_pattern, MAX_LINE_LENGTH, "%08lx", (unsigned long) addr);
	if (ret >= MAX_LINE_LENGTH) {
		printf("%s: Pattern is too long\n", __func__);
		exit(EXIT_FAILURE);
	}

	fp = fopen(smap_path, "r");
	if (!fp) {
		printf("%s: Failed to open file %s\n", __func__, smap_path);
		//exit(EXIT_FAILURE);
		return -1;
	}

	if (!check_for_pattern_addr(fp, addr_pattern, buffer))
		goto err_out;

	//Fetch the AnonHugePages: in the same block and check the number of hugepages.
	if (!check_for_pattern(fp, field.c_str(), buffer))
		goto err_out;

  field += "%10ld kB";
	if (sscanf(buffer, field.c_str(), &thp) != 1) {
		printf("Reading smap error\n");
		exit(EXIT_FAILURE);
	}

err_out:
	fclose(fp);
	return thp;
}

static tuple<unsigned long, unsigned long, unsigned long> check_mem() {
	uint64_t mem, total = 0, free = 0, avail = 0;
        string fields[3] = {"MemTotal:", "MemFree:", "MemAvailable:"};
	string field;
	const char* mem_path = "/proc/meminfo";
	FILE *fp;
	char buffer[MAX_LINE_LENGTH];

	fp = fopen(mem_path, "r");
	if (!fp) {
		printf("%s: Failed to open file %s\n", __func__, mem_path);
		exit(EXIT_FAILURE);
	}

        for (int i = 0; i < 3; i++) {
        	field = fields[i];
		if (!check_for_pattern(fp, field.c_str(), buffer)) goto err_out;

        	field += "%10ld kB";
		if (sscanf(buffer, field.c_str(), &mem) != 1) {
			printf("Reading meminfo error\n");
			exit(EXIT_FAILURE);
		}

		switch (i) {
			case 0:
				total = mem;
				break;
			case 1:
				free = mem;
				break;
			case 2:
				avail = mem;
				break;
		}
	}

err_out:
	fclose(fp);
	return make_tuple(total, free, avail);
}

static tuple<unsigned long, unsigned long> check_buddy_mem() {
	uint64_t mem_2mb = 0, mem_4mb = 0;
	const char* mem_path = "/proc/buddyinfo";
	string field = "Node 1, zone";
	FILE *fp;
	char buffer[MAX_LINE_LENGTH];
	vector<string> data, mem;

	fp = fopen(mem_path, "r");
	if (!fp) {
		printf("%s: Failed to open file %s\n", __func__, mem_path);
		exit(EXIT_FAILURE);
	}

	if (!check_for_pattern(fp, field.c_str(), buffer)) goto err_out;

	data = split(buffer, ' ');
	for (int d = 0; d < data.size(); d++) {
		string str = data.at(d);
		if (str.size() > 0) {
			mem.push_back(data.at(d));
		}
	}

	mem_2mb = stoul(mem.at(mem.size()-3), nullptr, 10);
	mem_4mb = stoul(mem.at(mem.size()-2), nullptr, 10);

err_out:
	fclose(fp);
	return make_tuple(mem_2mb, mem_4mb);
}

static void get_times(const char* stat_path, double* user_time, double* kernel_time) {
	FILE *fp = fopen(stat_path, "r");
	if (!fp) {
                printf("%s: Failed to open file %s\n", __func__, stat_path);
                exit(EXIT_FAILURE);
        }

	char buffer[MAX_LINE_LENGTH];
	fgets(buffer, MAX_LINE_LENGTH, fp);
	vector<string> data = split(buffer, ' ');
        *user_time = stod(data.at(13))/sysconf(_SC_CLK_TCK);
        *kernel_time = stod(data.at(14))/sysconf(_SC_CLK_TCK);
}

static void get_sizes(const char* stat_path, unsigned long* vm_size, unsigned long* rss) {
	FILE *fp = fopen(stat_path, "r");
	if (!fp) {
                printf("%s: Failed to open file %s\n", __func__, stat_path);
                exit(EXIT_FAILURE);
        }

	char buffer[MAX_LINE_LENGTH];
	fgets(buffer, MAX_LINE_LENGTH, fp);
	vector<string> data = split(buffer, ' ');
	*vm_size = stoul(data.at(22))/KB_SIZE;
	*rss = stoul(data.at(23))/KB_SIZE;
}

static void get_pfs(const char* stat_path, int* soft_pf, int* hard_pf) {
	FILE *fp = fopen(stat_path, "r");
	if (!fp) {
                printf("%s: Failed to open file %s\n", __func__, stat_path);
                exit(EXIT_FAILURE);
        }

	char buffer[MAX_LINE_LENGTH];
	fgets(buffer, MAX_LINE_LENGTH, fp);
	vector<string> data = split(buffer, ' ');
        *soft_pf = stoi(data.at(9));
        *hard_pf = stoi(data.at(11));
}

static void setup_pagemaps(int pid) {
	pmd_pagesize = read_pmd_pagesize();
	pagesize = getpagesize();
	pageshift = ffs(pagesize) - 1;

	if (snprintf(pagemap_proc, 255, pagemap_template, pid) < 0) {
		perror("get pagemap proc error");
		exit(EXIT_FAILURE);
	}

	pagemap_fd = open(pagemap_proc, O_RDONLY);
	if (pagemap_fd == -1) {
		perror("read pagemap:");
		exit(EXIT_FAILURE);
	}

	kpageflags_fd = open(kpageflags_proc, O_RDONLY);
	if (kpageflags_fd == -1) {
		perror("read kpageflags:");
		exit(EXIT_FAILURE);
	}
}

void split_pmd_thp(void) {
	char *one_page;
        size_t num_pmds = 8;
	size_t len = num_pmds * pmd_pagesize;
	uint64_t thp_size;
	size_t i;

	one_page = (char*) memalign(pmd_pagesize, len);

	if (!one_page) {
		printf("Fail to allocate memory\n");
		exit(EXIT_FAILURE);
	}

	madvise(one_page, len, MADV_HUGEPAGE);

	for (i = 0; i < len; i++) {
		one_page[i] = (char)i;
        }

	thp_size = check_huge(one_page);
	if (!thp_size) {
		printf("No THP is allocated\n");
		exit(EXIT_FAILURE);
	}

	/* split all THPs */
        for (i = 0; i < num_pmds; i++) {
		write_debugfs(PID_FMT, getpid(), (uint64_t)one_page + i * pmd_pagesize, (uint64_t)one_page + (i+1) * pmd_pagesize);
                thp_size = check_huge(one_page);
        }

	for (i = 0; i < len; i++)
		if (one_page[i] != (char)i) {
			printf("%ld byte corrupted\n", i);
			exit(EXIT_FAILURE);
		}


	thp_size = check_huge(one_page);
	if (thp_size) {
		printf("Still %ld kB AnonHugePages not split\n", thp_size);
		exit(EXIT_FAILURE);
	}

	printf("Split huge pages successful\n");
	free(one_page);
}

/*
int main(int argc, char **argv)
{
	if (geteuid() != 0) {
		printf("Please run the benchmark as root\n");
		exit(EXIT_FAILURE);
	}

	pagesize = getpagesize();
	pageshift = ffs(pagesize) - 1;
	pmd_pagesize = read_pmd_pagesize();

        cout << "pagesize = " << pagesize << ", pageshift = " << pageshift << ", pmd_pagesize = " << pmd_pagesize << endl;
	split_pmd_thp();

	return 0;
}
*/
