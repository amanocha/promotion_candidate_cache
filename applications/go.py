import argparse
import math
import os
import sys
import time

from subprocess import Popen, PIPE

HOME_DIR = "/home/aninda/promotion_candidate_cache/"
LAUNCH_DIR = HOME_DIR + "applications/launch/" 
MAIN_DIR = HOME_DIR + "results/"
GRAPH_DIR = HOME_DIR + "data/"
PROMOTION_CACHE_DIR = HOME_DIR + "pin3.7/source/tools/PromotionCache/output/promotion_data/"
DEMOTION_CACHE_DIR = HOME_DIR + "pin3.7/source/tools/PromotionCache/output/demotion_data/"

KB_PER_THP = 2048
CACHE_SIZE = 128
ACCESS_TIME = 30

REUSE_INTERVAL = 512
PROMOTION_FIXED = 21
HAWKEYE = 22
MADVISE_ALL = 50

NUM_REUSE = 7
NUM_PERCENTAGES = 7
NUM_THREADS = 4
NUM_SIZES = 9
NUM_BUCKETS = 10
TLB_SIZE = 3072
NUM_ITER = 3

mode = "motivation"
mode = "all"
mode = "frag"

# APPS
vp = ["bfs", "sssp", "pagerank"]
parsec = ["canneal", "dedup"]
spec = ["mcf", "omnetpp", "xalancbmk"]

if mode == "motivation" or mode == "all":
  apps = vp + ["canneal", "omnetpp", "xalancbmk", "dedup", "mcf"]
elif mode == "multiprocess":
  apps = ["pagerank", "mcf"]
  apps = ["pagerank", "sssp"]
else:
  apps = vp

multiprocess_pairs = [["pagerank_kron25", "mcf_mcf"]]
multiprocess_pairs = [["pagerank_kron25", "sssp_kron25"]]

# INPUTS
datasets = ["Kronecker_25", "Twitter", "Sd1_Arc"]
dataset_names = {"DBG_Kronecker_25/": "dbg_kron25",
            "DBG_Twitter/": "dbg_twit",
            "DBG_Sd1_Arc/": "dbg_web",
            "Kronecker_25/": "kron25",
            "Twitter/": "twit",
            "Sd1_Arc/": "web",
            "canneal_native.in": "canneal",
            "dedup_native.in": "dedup",
            "mcf_speed_inp.in": "mcf",
            "omnetpp.ini": "omnetpp",
            "t5.xml": "xalancbmk"}
plot_names = {"DBG_Kronecker_25/": "Sorted Kronecker 25",
            "DBG_Twitter/": "Sorted Twitter",
            "DBG_Sd1_Arc/": "Sorted Sd1 Arc Web",
            "DBG_Wikipedia/": "Sorted Wikipedia",
            "Kronecker_25/": "Kronecker 25",
            "Twitter/": "Twitter",
            "Sd1_Arc/": "Sd1 Arc Web",
            "Wikipedia/": "Wikipedia",
            "canneal_native.in": "canneal",
            "dedup_native.in": "dedup",
            "mcf_speed_inp.in": "mcf",
            "omnetpp.ini": "omnetpp",
            "t5.xml": "xalancbmk"}
app_names = {"bfs": "BFS",
             "sssp": "SSSP",
             "pagerank": "PR",
             "canneal": "canneal",
             "dedup": "dedup",
             "mcf": "mcf",
             "omnetpp": "omnetpp",
             "xalancbmk": "xalancbmk"}

vp_inputs = []
for dataset in datasets:
  vp_inputs += [dataset + "/", "DBG_" + dataset + "/"]
if apps == ["pagerank", "mcf"] or apps == ["pagerank", "sssp"]:
  vp_inputs = ["Kronecker_25/"]

canneal_inputs = ["canneal_native.in"]
dedup_inputs = ["dedup_native.in"]
mcf_inputs = ["mcf_speed_inp.in"]
omnetpp_inputs = ["omnetpp.ini"]
xalancbmk_inputs = ["t5.xml"]

inputs = {
         "bfs": vp_inputs,
         "sssp": vp_inputs,
         "pagerank": vp_inputs,
         "canneal": canneal_inputs,
         "dedup": dedup_inputs,
         "mcf": mcf_inputs,
         "omnetpp": omnetpp_inputs,
         "xalancbmk": xalancbmk_inputs
       }
mp_inputs = {"kron25": "Kronecker_25/", "twit": "Twitter/"}

start_seed = {"Kronecker_25/": "0", "Twitter/": "0", "Sd1_Arc/": "0",
              "DBG_Kronecker_25/": "3287496", "DBG_Twitter/": "15994127", "DBG_Sd1_Arc/": "18290613"}

def parse_args():
  parser = argparse.ArgumentParser()
  parser.add_argument("-a", "--app", type=str, help="Application to run (bfs, sssp, pagerank, canneal, dedup, mcf, omnetpp, xalancbmk)")
  parser.add_argument("-x", "--experiment", type=int, default=-1, help="Experiment to run (0-6)")
  parser.add_argument("-d", "--dataset", type=str, help="Dataset to run")
  args = parser.parse_args()
  return args

def run(cmd, tmp_output, output):
  print(cmd)
  exit = os.system(cmd)
  if not exit and "screen" not in cmd:
    if tmp_output != output:
      os.system("cp -r " + tmp_output + " " + output)
      os.system("rm -r " + tmp_output)
    print("Done! Navigate to " + output + "results.txt to see the results!")
  else:
    print("Multiprocess or experiment failed!")

# Experiment 1
def run_one(madvise):
  global GRAPH_DIR
  if madvise == 0:
    exp_name = "thp" if is_thp == 1 else "none"
  elif madvise == MADVISE_ALL:
    exp_name = "madvise_all"
  else:
    exp_name = "thp_" + str(madvise*5)
  exp_dir = RESULT_DIR + exp_name + "/"
  if not os.path.isdir(exp_dir):
    os.mkdir(exp_dir)
  for app in apps:
    if app in vp:
      source = "launch/" + app + "/main.cpp"
    else:
      source = "launch/" + app + "/" + app
    for input in inputs[app]:
      if app in vp:
        names = input.split("/")
        output = app + "_" + names[len(names)-2] + "/"
      else:
        output = app + "_" + input.split(".")[0] + "/"
      if not os.path.isdir(exp_dir + output):
        tmp_output = exp_dir + "tmp_" + output
        data = GRAPH_DIR + input
        cmd_args = ["time python3 measure.py", "-s", source, "-d", data, "-o", tmp_output, "-ma", str(madvise)]
        if "bfs" in app or "sssp" in app or "multiphase" in app:
          cmd_args += ["-ss", start_seed[input]]
        # NUM ITERATIONS
        cmd_args += ["-x", str(num_iter)]
        cmd = " ".join(cmd_args)
        run(cmd, tmp_output, exp_dir + output)

# Experiment 2: HawkEye or Promotion Cache    
def run_two(exp_type, config, size=512, access_time=30, num_threads=1, policy=0, demotion=False, promote_all=False):
  global GRAPH_DIR
  
  if "cache" in exp_type:
    exp_type += "_" + str(size) 
  exp_name = str(num_threads) + "_threads/" if num_threads > 1 else ""
  if config == 0:
    exp_name += "thp" if is_thp == 1 else "none"
    madvise = "0"
  else:
    exp_name += exp_type + "_" + str(config)
    if num_threads > 1:
      exp_name += "_" + str(policy)
    if demotion:
      exp_name += "_demote"
    if promote_all:
      exp_name += "_all"
    madvise = str(HAWKEYE)
  exp_dir = RESULT_DIR + str(access_time) + "_sec/" + exp_name + "/"
  if not os.path.isdir(exp_dir):
    os.makedirs(exp_dir)
  
  for app in apps:
    if app in vp:
      source = "launch/parallel/" + app + "/main.cpp" if num_threads > 1 else "launch/" + app + "/main.cpp" 
    else:
      source = "launch/" + app + "/" + app
    for input in inputs[app]:
      if app in vp:
        names = input.split("/")
        output = app + "_" + names[len(names)-2] + "/"
        data = GRAPH_DIR + input
      else:
        output = app + "/"
        data = GRAPH_DIR + app + "/" + input
      if not os.path.isdir(exp_dir + output):
        tmp_output = exp_dir + "tmp_" + output
        app_dir_name = app if app in vp else "other"

        single_thread = "single_thread/" + exp_type + "/" + str(access_time) + "_sec/" + app_dir_name 
        if promote_all:
          single_thread += "/promote_all"
        multithread = app_dir_name + "/promotion_" + exp_type + "_" + str(size) + "_" + str(num_threads) + "_" + str(config) + "_" + str(policy)

        promotion_dir = multithread if num_threads > 1 else single_thread
        demotion_dir = multithread if num_threads > 1 else single_thread
        promotion_data = PROMOTION_CACHE_DIR + promotion_dir + "/" + dataset_names[input] + "_" + str(config)
        demotion_data = DEMOTION_CACHE_DIR + demotion_dir + "/" + dataset_names[input]

        cmd_args = ["time python measure.py", "-s", source, "-d", data, "-o", tmp_output, "-pd", promotion_data, "-ma", madvise]
        
        if demotion:
          cmd_args += ["-dd", demotion_data]

        if num_threads > 1:
          cmd_args += ["-t", str(num_threads)]
        
        if "bfs" in app or "sssp" in app or "multiphase" in app:
          cmd_args += ["-ss", start_seed[input]]
        
        # NUM ITERATIONS
        num_iter = NUM_ITER
        cmd_args += ["-x", str(num_iter)]
        
        cmd = " ".join(cmd_args)
        run(cmd, tmp_output, exp_dir + output)

# Experiment 3: Multiprocess 
def run_three(exp_type, config, size=512, policy=0):
  global GRAPH_DIR
  if config == 0:
    exp_name = "thp" if is_thp == 1 else "none"
    madvise = "0"
  else:
    exp_name = exp_type + "_" + str(size) + "_" + str(config) + "_" + str(policy)
    madvise = str(HAWKEYE)
  for pair in multiprocess_pairs:
    exp_dir = RESULT_DIR + "_".join(pair) + "/" + exp_name + "/"
    if not os.path.isdir(exp_dir):
      os.makedirs(exp_dir)
    mid = 0
    for app_dataset in pair:
      app = app_dataset.split("_")[0]
      dataset = app_dataset.split("_")[1]
      if app in vp:
        source = "launch/" + app + "_multiprocess/main.cpp" 
      else:
        source = "launch/" + app + "_multiprocess/" + app
      if app in vp:
        input = mp_inputs[dataset]
        names = input.split("/")
        output = app + "_" + names[len(names)-2] + "/"
        data = GRAPH_DIR + input
      else:
        output = app + "/"
        data = GRAPH_DIR + app + "/" + inputs[app][0]
      if not os.path.isdir(exp_dir + output):
        tmp_output = exp_dir + "tmp_" + output

        promotion_dir = PROMOTION_CACHE_DIR + "multiprocess/" + "_".join(pair) + "/" + app + "/promotion_" + exp_type + "_" + str(size) + "_" + str(config) + "_" + str(policy)
        promotion_data = promotion_dir + "/" + dataset

        cmd_args = ["screen -dm time python measure.py", "-s", source, "-d", data, "-o", tmp_output, "-pd", promotion_data, "-ma", madvise, "-mid", str(mid)]
        if "bfs" in app or "sssp" in app or "multiphase" in app:
          cmd_args += ["-ss", start_seed[input]]
        # NUM ITERATIONS
        cmd_args += ["-x", str(NUM_ITER)]
        cmd = " ".join(cmd_args)
        run(cmd, tmp_output, exp_dir + output)
      mid += 1
    print("Running pair...", pair, config, size, policy)
    while (not os.path.isdir(exp_dir + output)):
      time.sleep(1)

def main():
  global apps, is_thp, RESULT_DIR, GRAPH_DIR, inputs, datasets, vp_inputs

  args = parse_args()

  stdout = Popen("uname -r", shell=True, stdout=PIPE).stdout
  output_str = stdout.read().decode("utf-8")

  stdout = Popen("cat /sys/kernel/mm/transparent_hugepage/enabled", shell=True, stdout=PIPE).stdout
  output_str1 = stdout.read().decode("utf-8")
  stdout = Popen("cat /sys/kernel/mm/transparent_hugepage/defrag", shell=True, stdout=PIPE).stdout
  output_str2 = stdout.read().decode("utf-8")
  if (output_str1.startswith("[always]") and output_str2.startswith("[always]")):
    is_thp = 1
  elif (output_str1.startswith("[always]") and not output_str2.startswith("[always]")):
    is_thp = 2
  else:
    is_thp = 0

  if args.dataset and args.app:
    datasets = [args.dataset]
    inputs[args.app] = [args.dataset + "/"]

  if args.app:
    apps = [args.app]

  if args.experiment == 0:
    RESULT_DIR = MAIN_DIR + "single_thread/"
    if not os.path.isdir(RESULT_DIR):
      os.mkdir(RESULT_DIR)
    run_two("cache", 0, 512)
  elif args.experiment == 1:
    RESULT_DIR += "madvise_prop/"
    if not os.path.isdir(RESULT_DIR):
      os.mkdir(RESULT_DIR)
    for i in range(0, 21):
      run_one(i)
  elif args.experiment == 2:
    RESULT_DIR = MAIN_DIR + "single_thread/"
    if not os.path.isdir(RESULT_DIR):
      os.mkdir(RESULT_DIR)
    run_two("hawkeye", 0, 512)
    for i in range(NUM_PERCENTAGES):
      percentage = int(math.pow(2, i))
      run_two("hawkeye", percentage, 512)
    run_two("hawkeye", 100, 512)
  elif args.experiment == 3:
    RESULT_DIR = MAIN_DIR + "single_thread/"
    if not os.path.isdir(RESULT_DIR):
      os.mkdir(RESULT_DIR)
    run_two("cache", 0, 512, access_time=ACCESS_TIME)
    for i in range(NUM_PERCENTAGES):
      percentage = int(math.pow(2, i))
      run_two("cache", percentage, CACHE_SIZE, access_time=ACCESS_TIME, promote_all=True)
    run_two("cache", 100, CACHE_SIZE, access_time=ACCESS_TIME, promote_all=True)
    for i in range(NUM_SIZES):
      size = int(math.pow(2, i+2))
      for f in range(NUM_PERCENTAGES):
        run_two("cache", int(math.pow(2, f)), size)
      run_two("cache", 100, size)
  elif args.experiment == 4:
    RESULT_DIR = MAIN_DIR + "multithread/"
    if not os.path.isdir(RESULT_DIR):
      os.mkdir(RESULT_DIR)
    for policy in range(2):
      for t in range(1, NUM_THREADS+1):
        num_threads = int(math.pow(2, t))
        run_two("cache", 0, 512, num_threads, policy)
        for i in range(NUM_PERCENTAGES):
          percentage = int(math.pow(2, i))
          run_two("cache", percentage, 512, num_threads, policy)
          run_two("cache", percentage, 512, num_threads, policy)
        run_two("cache", 100, 512, num_threads, policy)
        run_two("cache", 100, 512, num_threads, policy)
  elif args.experiment == 5:
    RESULT_DIR = MAIN_DIR + "frag50/"
    if not os.path.isdir(RESULT_DIR):
      os.mkdir(RESULT_DIR)
    run_two("cache", 0, CACHE_SIZE)
    run_two("cache", 100, CACHE_SIZE)
    run_two("hawkeye", 100, CACHE_SIZE)
    run_two("cache", 100, CACHE_SIZE, demotion=True)
  elif args.experiment == 6:
    RESULT_DIR = MAIN_DIR + "multiprocess/"
    if not os.path.isdir(RESULT_DIR):
      os.mkdir(RESULT_DIR)
    run_three("cache", 0, 512)
    for policy in range(2, 3):
      for i in range(NUM_PERCENTAGES):
        percentage = int(math.pow(2, i))
        run_three("cache", percentage, 512, policy)
      run_three("cache", 100, 512, policy)
    
if __name__ == "__main__":
  main()
