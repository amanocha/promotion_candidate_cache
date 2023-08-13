import argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter
import math
import os
import re
import sys

TLB_SIZE = 3072
MODE = "normal"
PERCENT = 100
NUM_4KB = 512
PROMOTION_DIR = "promotion_" + str(TLB_SIZE) + "/"

huge_pages = {}
totals = {}
regions = {}
rank_list = []
nodes = 0
edges = 0
total_num_huge_pages = 0
promotion_limit = 0

MAX_DIST = 0
MAX_FREQ = 0

def calc_footprint():
    global total_num_huge_pages

    if app == "bfs":
        total_num_huge_pages = 6*nodes + edges
    elif app == "sssp":
        total_num_huge_pages = 12*nodes + 2*edges
    elif app == "pagerank":
        total_num_huge_pages = 6*nodes + edges

    print("Total footprint = " + str(total_num_huge_pages) + " huge pages")

def parse_promotions(region_name, candidates):
    global huge_pages, totals

    promotion_file.write(region_name + "\n")
    for c in range(len(candidates)):
        candidate = candidates[c]
        time = candidate[0]
        offset = candidate[1]
        promotion_file.write(str(time) + "," + str(offset) + "\n")
        if time not in huge_pages:
            huge_pages[time] = {}
        if region_name not in huge_pages[time]:
            huge_pages[time][region_name] = []
        if offset not in huge_pages[time][region_name]:
            huge_pages[time][region_name].append(offset)

        if region_name not in totals:
            totals[region_name] = []
        if offset not in totals[region_name]:
            totals[region_name].append(offset)

def write_promotions():
    for time in sorted(huge_pages.keys()):
        stats_file.write("Time: " + str(time) + "\n")
        for region_name in huge_pages[time]:
            stats_file.write("\t" + region_name + ": promoting " + str(len(huge_pages[time][region_name])) + " huge pages\n")
        stats_file.write("\n")

    stats_file.write("Total:\n")
    for region_name in totals:
        stats_file.write("\t" + region_name + ": " + str(len(totals[region_name])) + "\n")
    stats_file.write("\n")

def rank_promotions():
    global rank_list

    rank_list = sorted(rank_list, reverse=True, key=lambda x: x[3])

    max = math.ceil(total_num_huge_pages*float(PERCENT)/100)
    for item in rank_list[0:max]:
        print(item)

    return

    huge_pages = []
    data = {}
    for i in range(len(rank_list)):
        item = rank_list[i]
        addr = item[0]
        region_name = item[1]

        if region_name not in data:
            data[region_name] = [] # x, y lists
        data[region_name].append(addr)

        candidate_name = region_name + "_" + str(addr)
        if candidate_name not in huge_pages:
            huge_pages.append(candidate_name)
        else:
            print("repeat candidate: ", candidate_name)
        if len(huge_pages) == max:
            break
    #print(huge_pages)

    for region_name in ["NODE_ARRAY", "EDGE_ARRAY", "PROP_ARRAY", "IN_WL", "OUT_WL", "VALS_ARRAY", "X_ARRAY", "IN_R_ARRAY"]:
        if region_name not in data:
            candidates = []
        else:
            candidates = data[region_name]
        write_promotions(region_name, candidates)

def process_file():
    global nodes, edges, regions, rank_list, total_num_huge_pages, promotion_limit, MAX_DIST, MAX_FREQ
    candidates = {}
    unique_candidates = []
    done_promoting = False

    filename = "cache_output/" + app + "/" + dataset
    if os.path.isfile(filename):
        print("READING: " + filename)
        data = open(filename)
        start = 0
        time = 0
        for line in data:
            region_match = re.match("(\w+): starting base = (.+), ending base = (.+)", line)
            if region_match != None:
                region_name = region_match.group(1)
                start = int(region_match.group(2), 16)
                end = int(region_match.group(3), 16)
                regions[region_name] = [start, end]
                if (region_name == "EDGE_ARRAY"):
                    edges = end-start
                elif (region_name == "PROP_ARRAY"):
                    nodes = end-start
                print(line)
            
            if (edges != 0 and nodes != 0 and total_num_huge_pages == 0):
                calc_footprint()
                promotion_limit = math.ceil(total_num_huge_pages*float(PERCENT)/100)
                print("Nodes = " + str(nodes) + ", Edges = " + str(edges) + ", Promotion limit = " + str(promotion_limit))
        
            time_match = re.match("(\d+): Memory Regions:", line)
            if time_match != None:
                time = int(time_match.group(1))

                #rank_promotions()
                #rank_list = []
                #print("\nTIME = " + str(time))
            
            match = re.match("\tbase = (.+), (\d+\.*\d*(?:e\+\d+)?), (\d+)", line)
            if match != None and not done_promoting:
                total_num_huge_pages += 1

                addr = int(match.group(1), 16)
                dist = float(match.group(2))
                freq = int(match.group(3))
            
                for region in regions:
                    if region not in candidates:
                        candidates[region] = []
                    start = regions[region][0]
                    end = regions[region][1]
                    if (addr >= start and addr < end):
                        addr_adj = addr-start
                        candidates[region].append([time, addr_adj])
                        rank_list.append([region, addr_adj, dist, freq])
                        name = region + "_" + str(addr_adj)
                        if name not in unique_candidates:
                            unique_candidates.append(name)
                            if (len(unique_candidates) >= promotion_limit):
                                done_promoting = True
                        break

                if (dist > MAX_DIST):
                    MAX_DIST = dist
                freq = int(match.group(3))
                if (freq > MAX_FREQ):
                    MAX_FREQ = freq

            if "Summary" in line:
                break

        data.close()

        for region in regions:
            if region in candidates:
                parse_promotions(region, candidates[region])
        write_promotions()

if __name__ == "__main__":
    filename = sys.argv[1]
    app = filename.split("/")[1]
    dataset = filename.split("/")[2]
    
    if (len(sys.argv) > 2):
        MODE = sys.argv[2]
        if (len(sys.argv) > 3):
            PERCENT = int(sys.argv[3])
    PROMOTION_DIR = app + "/promotion_" + MODE + "_" + str(PERCENT) + "/"
    print("MODE = " + MODE + ", PERCENT = " + str(PERCENT) + "\n")
    if (not os.path.isdir(PROMOTION_DIR)):
        os.mkdir(PROMOTION_DIR)

    promotion_filename = PROMOTION_DIR + dataset
    promotion_file = open(promotion_filename, "w+")
    stats_file = open(promotion_filename + "_stats", "w+")

    process_file()
    #rank_promotions()

    stats_file.write("Total num of candidates: " + str(total_num_huge_pages) + "\n")
    stats_file.write("Max reuse distance: " + str(MAX_DIST) + "\nMax freq: " + str(MAX_FREQ) + "\n")

    promotion_file.close()
    stats_file.close()
