import argparse
import numpy as np
import math
import os
import re
import sys

vp = ["bfs", "sssp", "pagerank"]
parsec = ["canneal", "dedup"]
spec = ["mcf", "omnetpp", "xalancbmk"]
npb = ["CG", "BT"]

# ARGS
MODE = ""
PERCENT = 100
POLICY = 0

HUGE_PAGE_SIZE = 2*1024*1024
NUM_4KB = 512
KB_SIZE = 1024
PROMOTION_DIR = "promotion/"
OFFSET = 0

total_num_huge_pages = 0
promotion_limit = 0

cache_match_str = "\tbase = (\w+), (\d+\.*\d*(?:e\+\d+)?), (\d+)(?:, (\d+))?"
hawkeye_match_str = "\tbase = (\w+), (\d+), bucket = (\d+)"

MAX_DIST = 0
MAX_FREQ = 0

unique_candidates = []
to_promote = []
done_promoting = False

def promote(addr):
    global unique_candidates, to_promote, done_promoting

    if addr not in unique_candidates:
        unique_candidates.append(addr)
        to_promote.append(addr)
        if (len(unique_candidates) >= promotion_limit):
            done_promoting = True

def write_promotions(candidates):
    global to_promote, done_promoting

    done_promoting = False
    for time in sorted(candidates.keys()):
        if done_promoting:
            break
        stats_file.write("Time: " + str(time) + ", Promotions: " + str(len(candidates[time])) + "\n")
        tids = sorted(candidates[time].keys())
        while (not done_promoting and len(candidates[time]) > 0):
            if (POLICY == 0):
                for tid in tids:
                    if len(candidates[time]) > 0 and tid in candidates[time] and len(candidates[time][tid]) > 0:
                        tup = candidates[time][tid].pop(0)
                        addr = tup[0]
                        freq = tup[1]
                        #print(time, tid, addr, freq)
                        promote(addr)
                        if (done_promoting):
                            break
                    elif tid in candidates[time] and len(candidates[time][tid]) == 0:
                        del candidates[time][tid]
            else:
                if len(candidates[time]) > 0:
                    tups = [[tid] + candidates[time][tid][0] for tid in tids if tid in candidates[time] and len(candidates[time][tid]) > 0]
                    max_tid = sorted(tups, key = lambda x: x[2], reverse=True)[0][0]
                    tup = candidates[time][max_tid].pop(0)
                    addr = tup[0]
                    freq = tup[1]
                    #print(time, max_tid, addr, freq)
                    promote(addr)
                    if (done_promoting):
                        break
                    if len(candidates[time][max_tid]) == 0:
                        del candidates[time][max_tid]
        '''
        '''
        to_promote.sort()
        for addr in to_promote:
            promotion_file.write(str(time) + "," + str(addr+OFFSET) + "\n")
        stats_file.write("\n")
        to_promote = []
    if (len(unique_candidates) < promotion_limit):
      print("DID NOT PROMOTE ENOUGH PAGES")

    stats_file.write("Total huge pages promoted: " + str(promotion_limit) + "\n")

def process_file(filename):
    global total_num_huge_pages, promotion_limit, MAX_DIST, MAX_FREQ

    reading = False
    candidates = {}
    times = {}
    time = 0

    if os.path.isfile(filename):
        print("READING: " + filename)
        data = open(filename)
        size = 0
        for line in data:
            mem_region_match = re.match("NODE_ARRAY: starting base = (.*), ending base = (.*)", line)
            if mem_region_match != None:
                print("MEM REGION: " + mem_region_match.group(1) + " - " + mem_region_match.group(2))
                reading = True

            if app not in vp:
                reading = True

            footprint_match = re.match("footprint: start = (\d+)KB, end = (\d+)KB, diff = (\d+)KB", line)
            if footprint_match != None:
                size = int(footprint_match.group(2))*KB_SIZE

            time_match = re.match("(\d+): Memory Regions(?: \(Cache #(\d+)\))?:", line)
            if time_match != None:
                #time = int(time_match.group(1))
                tid = int(time_match.group(2))
                if tid not in times:
                    times[tid] = 0
                else:
                    times[tid] += 1
                time = times[tid]

                #rank_promotions()
                #rank_list = []
                #print("\nTIME = " + str(time))
            
            match_str = cache_match_str if "cache" in MODE else hawkeye_match_str
            match = re.match(match_str, line)
            if match != None and reading == True:
                total_num_huge_pages += 1

                addr = int(match.group(1), 16)
                dist = float(match.group(2))
                freq = int(match.group(3))
                cache_freq = int(match.group(4))
                print(addr, dist, freq, screenline)
            
                if time not in candidates:
                    candidates[time] = {tid: []}
                if tid not in candidates[time]:
                    candidates[time][tid] = []
                candidates[time][tid].append([addr, cache_freq])

                if (dist > MAX_DIST):
                    MAX_DIST = dist
                if (freq > MAX_FREQ):
                    MAX_FREQ = freq

        total_num_huge_pages = int((size+HUGE_PAGE_SIZE-1)/HUGE_PAGE_SIZE)
        promotion_limit = int(total_num_huge_pages*PERCENT/100)
        print("Total footprint = " + str(total_num_huge_pages) + " huge pages, promoting " + str(promotion_limit) + "\n")
        data.close()

        write_promotions(candidates)

if __name__ == "__main__":
    filename = sys.argv[1]
    app = filename.split("/")[-2]
    app = "other"
    dataset = filename.split("/")[-1]

    if (len(sys.argv) > 2):
        MODE = sys.argv[2]
        num_threads = MODE.split("_")[-1]
        if (len(sys.argv) > 3):
            PERCENT = int(sys.argv[3])
            if (len(sys.argv) > 4):
                POLICY = int(sys.argv[4])
    PROMOTION_DIR = app + "/promotion_" + MODE + "_" + str(PERCENT) + "_" + str(POLICY) + "/"
    print("MODE = " + MODE + ", PERCENT = " + str(PERCENT) + ", POLICY = " + str(POLICY) + ", OUTPUT DIR = " + str(PROMOTION_DIR))

    if app in vp or app == "canneal":
        if num_threads == "16":
            OFFSET = 191
        else:
            OFFSET = 190
    else: # dedup, mcf, omnetpp, xalancbmk
        OFFSET = 0

    if (not os.path.isdir(PROMOTION_DIR)):
        os.mkdir(PROMOTION_DIR)
    promotion_filename = PROMOTION_DIR + dataset
    promotion_file = open(promotion_filename, "w+")
    stats_file = open(promotion_filename + "_stats", "w+")

    process_file(filename)
    #rank_promotions()

    stats_file.write("Total num of candidates: " + str(total_num_huge_pages) + "\n")
    if ("cache" in MODE):
        stats_file.write("Max reuse distance: " + str(MAX_DIST) + "\nMax freq: " + str(MAX_FREQ) + "\n")

    promotion_file.close()
    stats_file.close()
