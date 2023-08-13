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
MAX_PROMOTIONS_PER_TIME = 512

total_num_huge_pages = 0
promotion_limit = 0
promotion_limit1 = 0
promotion_limit2 = 0

cache_match_str = "\tbase = (\w+), (\d+\.*\d*(?:e\+\d+)?), (\d+)(?:, (\d+))?"
hawkeye_match_str = "\tbase = (\w+), (\d+), bucket = (\d+)"

MAX_DIST = 0
MAX_FREQ = 0

candidates = {}
unique_candidates = []
to_promote1 = []
to_promote2 = []
done_promoting = False

def promote(time, addr, app):
    global unique_candidates, to_promote1, to_promote2, done_promoting, promotion_limit1, promotion_limit2

    candidate_id = str(addr)+"_"+app
    if candidate_id not in unique_candidates:
        unique_candidates.append(candidate_id)
        if app == app1+"_"+dataset1:
            to_promote1.append([time, addr])
            promotion_limit1 += 1
        else:
            to_promote2.append([time, addr])
            promotion_limit2 += 1
        if (len(unique_candidates) >= promotion_limit):
            done_promoting = True

def write_promotions(candidates):
    global to_promote1, to_promote2, done_promoting

    done_promoting = False
    for time in sorted(candidates.keys()):
        local_time = time
        promotions_per_time = 0
        done_time = False
        if done_promoting:
            break
        #stats_file.write("Time: " + str(time) + ", Promotions: " + str(len(candidates[time])) + "\n")
        apps = sorted(candidates[time].keys())
        while (not done_promoting and len(candidates[time]) > 0 and not done_time):
            if (POLICY == 0):
                for app in apps:
                    if len(candidates[time]) > 0 and app in candidates[time] and len(candidates[time][app]) > 0:
                        tup = candidates[time][app].pop(0)
                        addr = tup[0]
                        freq = tup[1]
                        #print(local_time, app, addr, freq)
                        promote(local_time, addr, app)
                        promotions_per_time += 1 
                        if (done_promoting):
                            break
                        if promotions_per_time == MAX_PROMOTIONS_PER_TIME:
                            local_time += 1
                    elif app in candidates[time] and len(candidates[time][app]) == 0:
                        del candidates[time][app]
            elif (POLICY == 1):
                if len(candidates[time]) > 0:
                    tups = [[app] + candidates[time][app][0] for app in apps if app in candidates[time] and len(candidates[time][app]) > 0]
                    max_app = sorted(tups, key = lambda x: x[2], reverse=True)[0][0]
                    tup = candidates[time][max_app].pop(0)
                    addr = tup[0]
                    freq = tup[1]
                    #print(time, max_app, addr, freq)
                    promote(local_time, addr, max_app)
                    promotions_per_time += 1 
                    if (done_promoting):
                        break
                    if promotions_per_time == MAX_PROMOTIONS_PER_TIME:
                        local_time += 1
                    if len(candidates[time][max_app]) == 0:
                        del candidates[time][max_app]
            else:
                for app in apps:
                    if len(candidates[time]) > 0 and app in candidates[time] and len(candidates[time][app]) > 0:
                        tup = candidates[time][app].pop(0)
                        addr = tup[0]
                        freq = tup[1]
                        print(local_time, app, addr, freq)
                        promote(local_time, addr, app)
                        promotions_per_time += 1 
                        if (done_promoting):
                            break
                        if promotions_per_time == MAX_PROMOTIONS_PER_TIME:
                            local_time += 1
                    elif app in candidates[time] and len(candidates[time][app]) == 0:
                        del candidates[time][app]
                        for remaining_app in candidates[time]:
                            if (time+2) not in candidates:
                                candidates[time+2] = {remaining_app: []}
                            if remaining_app in candidates[time+2]:
                                candidates[time+2][remaining_app] = sorted(candidates[time][remaining_app] + candidates[time+2][remaining_app], key = lambda x: x[1], reverse=True)
                            else:
                                candidates[time+2][remaining_app] = candidates[time][remaining_app]
                            done_time = True
                        break
        '''
        '''
        
        to_promote1 = sorted(to_promote1, key = lambda x: (x[0], x[1]))
        to_promote2 = sorted(to_promote2, key = lambda x: (x[0], x[1]))
        for tup in to_promote1:
            promotion_file1.write(str(tup[0]) + "," + str(tup[1]) + "\n")
        for tup in to_promote2:
            promotion_file2.write(str(tup[0]) + "," + str(tup[1]) + "\n")
        stats_file1.write("\n")
        stats_file2.write("\n")
        to_promote1 = []
        to_promote2 = []
    if (len(unique_candidates) < promotion_limit):
      print("DID NOT PROMOTE ENOUGH PAGES: " + str(len(unique_candidates)) + "/" + str(promotion_limit))

    stats_file1.write("Total huge pages promoted: " + str(promotion_limit1) + "\n")
    stats_file2.write("Total huge pages promoted: " + str(promotion_limit2) + "\n")

def process_file(filename, app, dataset):
    global candidates, MAX_DIST, MAX_FREQ

    reading = False
    time = -1
        
    if app in vp or app == "canneal":
        offset = 187
        if app == "pagerank":
            offset = 189
    else: # dedup, mcf, omnetpp, xalancbmk
        offset = 0

    app_name = app + "_" + dataset
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
                time += 2

                #rank_promotions()
                #rank_list = []
                #print("\nTIME = " + str(time))
            
            match_str = cache_match_str if "cache" in MODE else hawkeye_match_str
            match = re.match(match_str, line)
            if match != None and reading == True:

                addr = int(match.group(1), 16)
                dist = float(match.group(2))
                freq = int(match.group(3))
                cache_freq = int(match.group(4))
            
                if time not in candidates:
                    candidates[time] = {app_name: []}
                if app_name not in candidates[time]:
                    candidates[time][app_name] = []
                candidates[time][app_name].append([addr+offset, cache_freq])

                if (dist > MAX_DIST):
                    MAX_DIST = dist
                if (freq > MAX_FREQ):
                    MAX_FREQ = freq

        val = int((size+HUGE_PAGE_SIZE-1)/HUGE_PAGE_SIZE)
        data.close()
        return val


if __name__ == "__main__":
    # APP 1
    filename1 = sys.argv[1]
    app1 = filename1.split("/")[-2]
    dataset1 = filename1.split("/")[-1]
    if app1 == "other":
        app1 = dataset1
    
    # APP 2
    filename2 = sys.argv[2]
    app2 = filename2.split("/")[-2]
    dataset2 = filename2.split("/")[-1]
    if app2 == "other":
        app2 = dataset2

    if (len(sys.argv) > 3):
        MODE = sys.argv[3]
        if (len(sys.argv) > 4):
            PERCENT = int(sys.argv[4])
            if (len(sys.argv) > 5):
                POLICY = int(sys.argv[5])
    print("MODE = " + MODE + ", PERCENT = " + str(PERCENT) + ", POLICY = " + str(POLICY) + "\n")

    for app, dataset in zip([app1, app2], [dataset1, dataset2]):
        PROMOTION_DIR = "multiprocess/" + app1 + "_" + dataset1 + "_" + app2 + "_" + dataset2 + "/" + app + "/promotion_" + MODE + "_" + str(PERCENT) + "_" + str(POLICY) + "/" 
        if (not os.path.isdir(PROMOTION_DIR)):
            os.makedirs(PROMOTION_DIR)
        if app == app1 and dataset == dataset1:
            promotion_filename1 = PROMOTION_DIR + dataset
            print(promotion_filename1)
        else:
            promotion_filename2 = PROMOTION_DIR + dataset
            print(promotion_filename2)
    promotion_file1 = open(promotion_filename1, "w+")
    promotion_file2 = open(promotion_filename2, "w+")
    stats_file1 = open(promotion_filename1 + "_stats", "w+")
    stats_file2 = open(promotion_filename2 + "_stats", "w+")

    for filename, app, dataset in zip([filename1, filename2], [app1, app2], [dataset1, dataset2]):
        total_num_huge_pages += process_file(filename, app, dataset)
        
    promotion_limit = int(total_num_huge_pages*PERCENT/100)
    print("Total footprint = " + str(total_num_huge_pages) + " huge pages, promoting " + str(promotion_limit) + "\n")
    
    write_promotions(candidates)

    promotion_file1.close()
    promotion_file2.close()
    stats_file1.close()
    stats_file2.close()
