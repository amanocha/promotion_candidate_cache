import argparse
import numpy as np
import os
import re
import sys

NUM_ITER = 5

graph = ["kron25", "twit", "web", "dbg_kron25", "dbg_twit", "dbg_web"]
other = ["canneal", "dedup", "mcf", "omnetpp", "xalancbmk"]
datasets = graph + other

def process_file(dataset):
    times = []
    access = 0

    for iter in range(NUM_ITER):
        filename = "time_output/" + dataset + "_" + str(iter)
        if os.path.isfile(filename):
            #print("READING: " + filename)
            data = open(filename)
            for line in data:
                match = re.match("total kernel computation time: (\d+\.*\d*(?:e\+\d+)?)", line)
                if match != None:
                    time = float(match.group(1))
                    times.append(time)
                else:
                    match = re.match("(\d+\.*\d*(?:e\+\d+)?)user (\d+\.*\d*(?:e\+\d+)?)system (\d+):(\d+\.*\d*(?:e\+\d+)?)elapsed", line)
                    if match != None:
                        minutes = int(match.group(3))
                        seconds = float(match.group(4))
                        time = minutes*60.0 + seconds
                        times.append(time)
            data.close()

    #print(times)

    filename = "access_output/" + dataset
    if os.path.isfile(filename):
        #print("READING: " + filename)
        data = open(filename)
        for line in data:
            match = re.match("Total Accesses = (\d+)", line)
            if match != None:
                access = int(match.group(1))
        data.close()
    #print(access)

    time_avg = sum(times)/len(times)
    accesses_per_sec = access/float(time_avg)

    print(dataset + ": " + str(int(accesses_per_sec)))

if __name__ == "__main__":
    for dataset in datasets:
        process_file(dataset)
