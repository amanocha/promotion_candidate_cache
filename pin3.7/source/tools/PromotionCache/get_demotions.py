import argparse
import numpy as np
import math
import os
import re
import sys

vp = ["bfs", "sssp", "pagerank"]
parsec = ["canneal", "dedup"]
spec = ["mcf", "omnetpp", "xalancbmk"]

HUGE_PAGE_SIZE = 2*1024*1024
MODE = ""
NUM_4KB = 512
KB_SIZE = 1024
DEMOTION_DIR = "demotion/"
OFFSET = 0

total_num_huge_pages = 0

cache_match_str = "\tbase = (\w+), (\d+\.*\d*(?:e\+\d+)?), (\d+)(?:, (\d+))?"
hawkeye_match_str = "\tbase = (\w+), (\d+\.*\d*(?:e\+\d+)?), bucket = (\d+)"

MAX_DIST = 0
MAX_FREQ = 0

def write_demotions(candidates):
    promoted_addr = []
    demoted_addr = []
    to_demote = []
    for time in sorted(candidates.keys()):
        stats_file.write("Time: " + str(time) + ", Demotions: " + str(len(candidates[time])) + "\n")
        for (addr, freq) in candidates[time]:
            if addr not in promoted_addr:
                promoted_addr.append(addr) # promote only once
            else:
                if freq > -1 and addr not in demoted_addr:
                    print("demoting:", addr, freq)
                    demoted_addr.append(addr)
                    to_demote.append(addr)
        to_demote.sort()
        for addr in to_demote:
            demotion_file.write(str(time) + "," + str(addr+OFFSET) + "\n")
        stats_file.write("\n")
        to_demote = []

def process_file(filename):
    global total_num_huge_pages, MAX_DIST, MAX_FREQ

    reading = False
    candidates = {}

    if os.path.isfile(filename):
        print("READING: " + filename)
        data = open(filename)
        size = 0
        for line in data:
            mem_region_match = re.match("NODE_ARRAY: starting base = (.*), ending base = (.*)", line)
            if mem_region_match != None:
                print("MEM REGION: " + mem_region_match.group(1) + " - " + mem_region_match.group(2))
                reading = True

            if app == "sssp" or app not in vp:
                reading = True

            footprint_match = re.match("footprint: start = (\d+)KB, end = (\d+)KB, diff = (\d+)KB", line)
            if footprint_match != None:
                size = int(footprint_match.group(2))*KB_SIZE

            time_match = re.match("(\d+): Memory Regions(?: \(Cache #(\d+)\))?:", line)
            if time_match != None:
                time = int(time_match.group(1))
            
            match_str = cache_match_str if "cache" in MODE else hawkeye_match_str
            match = re.match(match_str, line)
            if match != None and reading == True:
                total_num_huge_pages += 1

                if "cache" in MODE:
                    addr = int(match.group(1), 16)
                    dist = float(match.group(2))
                    freq = int(match.group(3))
                    cache_freq = int(match.group(4))
            
                    if cache_freq > 0:
                        if time not in candidates:
                            candidates[time] = []
                        candidates[time].append((addr, cache_freq))

                        if (dist > MAX_DIST):
                            MAX_DIST = dist
                        if (freq > MAX_FREQ):
                            MAX_FREQ = freq
                else:
                    addr = int(match.group(1), 16)
                    coverage = float(match.group(2))
                    bucket = int(match.group(3))

                    if time not in candidates:
                        candidates[time] = []
                    candidates[time].append(addr)

        total_num_huge_pages = int((size+HUGE_PAGE_SIZE-1)/HUGE_PAGE_SIZE)
        print("Total footprint = " + str(total_num_huge_pages) + " huge pages")
        print("----------\n")
        data.close()

        write_demotions(candidates)

if __name__ == "__main__":
    filename = sys.argv[1]
    app = filename.split("/")[-2]
    dataset = filename.split("/")[-1]

    MODE = "cache" if "cache" in filename else "other"
    
    if app in vp:
        if app == "sssp":
            if "web" in dataset:
                OFFSET = 190
            else:
                OFFSET = 189
        if app == "bfs" or app == "pagerank":
            if "kron" in dataset:
                OFFSET = 189
            else:
                OFFSET = 190
        if app == "multiphase":
            OFFSET = 190
    elif dataset == "canneal" and MODE == "cache":
        OFFSET = 397426688
        OFFSET = 0
    elif dataset == "canneal":
        OFFSET = 397234176 #67100397
    elif dataset == "omnetpp":
        OFFSET = -368
        OFFSET = 0
    elif dataset == "xalancbmk":
        OFFSET = -352
        OFFSET = 0
    else: # dedup, mcf
        OFFSET = 0

    if (len(sys.argv) > 2):
        PERCENT = int(sys.argv[2])
        if (len(sys.argv) > 3):
            promote_all = sys.argv[3]
            if (len(sys.argv) > 4):
                OFFSET = int(sys.argv[4])

    DEMOTION_DIR = filename.replace("output/", "output/demotion_data/").replace(dataset, "")
    print("MODE = " + MODE + ", OFFSET = " + str(OFFSET) + ", PROMOTION DIR = " + DEMOTION_DIR + "\n")

    if (not os.path.isdir(DEMOTION_DIR)):
        os.makedirs(DEMOTION_DIR, exist_ok=True)
    demotion_filename = DEMOTION_DIR + dataset
    demotion_file = open(demotion_filename, "w+")
    stats_file = open(demotion_filename + "_stats", "w+")

    process_file(filename)

    stats_file.write("Total num of candidates: " + str(total_num_huge_pages) + "\n")
    if ("cache" in MODE):
        stats_file.write("Max reuse distance: " + str(MAX_DIST) + "\nMax freq: " + str(MAX_FREQ) + "\n")

    demotion_file.close()
    stats_file.close()
