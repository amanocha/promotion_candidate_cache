import argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter
import os
import re
import sys

TLB_SIZE = 1024
TLB_SIZE_1G = 4
NUM_4KB = 512
xlabel = {"4kb": "4KB Virtual Page Number", "2mb": "2MB Virtual Page Number"}
title = {"NODE_ARRAY": "Vertex Index Array", "EDGE_ARRAY": "Edge Array", "PROP_ARRAY": "Vertex Property Array", "IN_WL": "Worklist 1", "OUT_WL": "Worklist 2", "All Data": "All Data"}

dataset = ""
rank_list = []

def process_file(region_name):
    global rank_list
    reuse_dist = {}
    gb_reuse_dist = {}

    filename = filepath + "/1gb"
    if os.path.isfile(filename):
        print("READING: " + filename)
        data = open(filename)
        reading = False
        start = 0
        for line in data:
            if region_name in line and not reading:
                reading = True
                match = re.match(region_name + ": starting base = (.+), ending base = (.+)", line)
                start = int(match.group(1), 16)
                print(line)
            elif "combined average" in line and reading:
                reading = False
            if reading and line.startswith("\t"):
                match = re.match("\tbase = (.+), reuse dist = (\d+\.*\d*(?:e\+\d+)?), n = (\d+)", line)
                addr = int(match.group(1), 16)
                dist = float(match.group(2))
                gb_reuse_dist[addr-start] = dist
        data.close()

    filename = filepath + "/2mb"
    if os.path.isfile(filename):
        print("READING: " + filename)
        data = open(filename)
        reading = False
        start = 0
        for line in data:
            if region_name in line and not reading:
                reading = True
                match = re.match(region_name + ": starting base = (.+), ending base = (.+)", line)
                start = int(match.group(1), 16)
                print(line)
            elif "combined average" in line and reading:
                reading = False
            if reading and line.startswith("\t"):
                match = re.match("\tbase = (.+), reuse dist = (\d+\.*\d*(?:e\+\d+)?), n = (\d+)", line)
                addr = int(match.group(1), 16)
                dist = float(match.group(2))
                reuse_dist[addr-start] = dist
        data.close()

    filename = filepath + "/4kb"
    if os.path.isfile(filename):
        print("READING: " + filename)
        data = open(filename)
        reading = False
        start = 0
        start_4kb = 0
        reg_x, reg_y, reg_addr, reg_freq, upgrade_x, upgrade_y, upgrade_addr, upgrade_freq, sparse_x, sparse_y, sparse_addr, sparse_freq, one_g_x, one_g_y, one_g_addr, one_g_freq = ([] for i in range(16))
        for line in data:
            if region_name in line and not reading:
                reading = True
                match = re.match(region_name + ": starting base = (.+), ending base = (.+)", line)
                #start = int(int(match.group(1), 16)/NUM_4KB)
                start_4kb = int(match.group(1), 16)
                print(line)
            elif "combined average" in line and reading:
                reading = False
            if reading and line.startswith("\t"):
                match = re.match("\tbase = (.+), reuse dist = (\d+\.*\d*(?:e\+\d+)?), n = (\d+)", line)
                addr = int(match.group(1), 16)
                dist = float(match.group(2))
                freq = int(match.group(3))
                hp_addr = int((addr-start_4kb)/NUM_4KB)
                hp_addr_1g = int((addr-start_4kb)/NUM_4KB/NUM_4KB)

                x_val = dist #addr-start_4kb
                y_val = reuse_dist[hp_addr] #dist-reuse_dist[hp_addr]
                if (y_val < 0):
                    y_val = 0

                if dist >= TLB_SIZE:
                    if reuse_dist[hp_addr] < TLB_SIZE:
                        upgrade_x.append(x_val)
                        upgrade_y.append(y_val)
                        upgrade_addr.append(addr-start_4kb)
                        upgrade_freq.append(freq)

                        rank_list.append([dist-reuse_dist[hp_addr], addr-start_4kb, region_name])
                    else:
                        if gb_reuse_dist[hp_addr_1g] < TLB_SIZE_1G:
                            one_g_x.append(x_val)
                            one_g_y.append(y_val)
                            one_g_addr.append(addr-start_4kb)
                            one_g_freq.append(freq)
                        else:
                            sparse_x.append(x_val)
                            sparse_y.append(y_val)
                            sparse_addr.append(addr-start_4kb)
                            sparse_freq.append(freq)
                else:
                    reg_x.append(x_val)
                    reg_y.append(y_val)
                    reg_addr.append(addr-start_4kb)
                    reg_freq.append(freq)
        data.close()

        return reg_x, reg_y, reg_addr, reg_freq, upgrade_x, upgrade_y, upgrade_addr, upgrade_freq, sparse_x, sparse_y, sparse_addr, sparse_freq, one_g_x, one_g_y, one_g_addr, one_g_freq

def plot(x, y, upgrade_x, upgrade_y, sparse_x, sparse_y, one_g_x, one_g_y, region_name):
    fig = plt.figure(1, figsize=(20.0, 8.0))
    dot_size = 100
    plt.scatter(x, y, s=dot_size, color='green', label='High TLB Hit Rate with 4KB')
    plt.scatter(upgrade_x, upgrade_y, s=dot_size, color='blue', label='Low TLB Hit Rate with 4KB, High TLB Hit Rate with 2MB')
    plt.scatter(sparse_x, sparse_y, s=dot_size, color='red', label='Low TLB Hit Rate with 4KB and 2MB')
    #plt.scatter(one_g_x, one_g_y, s=dot_size, color='purple', label='Low TLB Hit Rate with 4KB and 2MB, High TLB Hit Rate with 1GB')
    plt.axvline(x=1024, color='black', linestyle='dashed', linewidth=1)
    plt.axhline(y=1024, color='black', linestyle='dashed', linewidth=1)

    xmin, xmax, ymin, ymax = plt.axis()
    plt.xlim(1, xmax)
    plt.ylim(1, 1e10)

    plt.xscale("log")
    plt.yscale("log")
    plt.xticks(fontsize=30)
    plt.yticks(fontsize=30)
    plt.xlabel("4KB Page Reuse Distance", fontsize=35)
    plt.ylabel("2MB Page Reuse Distance", fontsize=35)
    plt.title("2MB vs. 4KB Page Reuse Distance", fontsize=40)
    plt.legend(loc='upper center', fontsize=30)

    
    dir = filepath.replace("data", "figs")
    if (not os.path.isdir(dir.replace(dataset + "/", ""))):
        os.mkdir(dir.replace(dataset + "/", ""))
    if (not os.path.isdir(dir)):
        os.mkdir(dir)
    print("File path: " + dir + "/" + dataset + "_" + region_name.lower() + ".png")
    fig.savefig(dir + "/" + dataset + "_" + region_name.lower() + ".png", bbox_inches='tight')
    fig.savefig(dir + "/" + dataset + "_" + region_name.lower() + ".pdf", bbox_inches='tight')
    plt.clf()
    print("Done!\n")

if __name__ == "__main__":
    filepath = sys.argv[1]
    dataset = filepath.split("/")[2]
    if (len(sys.argv) > 2):
        TLB_SIZE = int(sys.argv[2])
    print("dataset = " + str(dataset) + ", TLB size = " + str(TLB_SIZE))

    tot_reg_x, tot_reg_y, tot_upgrade_x, tot_upgrade_y, tot_sparse_x, tot_sparse_y, tot_one_g_x, tot_one_g_y = ([] for i in range(8))
    for region_name in ["NODE_ARRAY", "EDGE_ARRAY", "PROP_ARRAY", "IN_WL", "OUT_WL"]:
        reg_x, reg_y, reg_addr, reg_freq, upgrade_x, upgrade_y, upgrade_addr, upgrade_freq, sparse_x, sparse_y, sparse_addr, sparse_freq, one_g_x, one_g_y, one_g_addr, one_g_freq = process_file(region_name)
        plot(reg_x, reg_y, upgrade_x, upgrade_y, sparse_x, sparse_y, one_g_x, one_g_y, region_name)
        #plot_freq(reg_addr, reg_freq, upgrade_addr, upgrade_freq, sparse_addr, sparse_freq, region_name)
        tot_reg_x += reg_x
        tot_reg_y += reg_y
        tot_upgrade_x += upgrade_x
        tot_upgrade_y += upgrade_y
        tot_sparse_x += sparse_x
        tot_sparse_y += sparse_y
        tot_one_g_x += one_g_x
        tot_one_g_y += one_g_y

    plot(tot_reg_x, tot_reg_y, tot_upgrade_x, tot_upgrade_y, tot_sparse_x, tot_sparse_y, tot_one_g_x, tot_one_g_y, "All Data")
