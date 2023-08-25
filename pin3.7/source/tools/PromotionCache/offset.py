import numpy as np
import os
import re
import sys

HUGE_PAGE_SIZE = 2*1024*1024
NUM_4KB = 512
KB_SIZE = 1024
PROMOTION_DIR = "promotion/"

vp = ["bfs", "sssp", "pagerank"]
parsec = ["canneal", "dedup"]
spec = ["mcf", "omnetpp", "xalancbmk"]

def get_offset(app, dataset) {
    if app in vp:
        if app == "sssp":
            if "web" in dataset:
                offset = 190
            else:
                offset = 189
        if app == "bfs" or app == "pagerank":
            if "kron" in dataset:
                offset = 189
            else:
                offset = 190
        if app == "multiphase":
            offset = 190
    elif dataset == "canneal" and MODE == "cache":
        offset = 397426688
        offset = 0
    elif dataset == "canneal":
        offset = 397234176 #67100397
    elif dataset == "omnetpp":
        offset = -368
        offset = 0
    elif dataset == "xalancbmk":
        offset = -352
        offset = 0
    else: # dedup, mcf
        offset = 0

    return offset
}