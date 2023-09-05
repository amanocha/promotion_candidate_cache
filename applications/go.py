import argparse
import math
import os
import re
import sys
import time

from subprocess import Popen, PIPE

KB_PER_THP = 2048
PCC_SIZE = 128
ACCESS_TIME = 30

PROMOTION = 21

NUM_PERCENTAGES = 7
NUM_THREADS = 4
NUM_SIZES = 9
NUM_POLICIES = 2
NUM_ITER = 3

# APPS
vp = ["bfs", "sssp", "pagerank"]
parsec = ["canneal", "dedup"]
spec = ["mcf", "omnetpp", "xalancbmk"]
apps = vp + parsec + spec
apps = ["bfs", "dedup"]

# INPUTS
datasets = ["Kronecker_25", "Twitter", "Sd1_Arc"]
dataset_names = {"DBG_Kronecker_25/": "dbg_kron25",
            "DBG_Twitter/": "dbg_twit",
            "DBG_Sd1_Arc/": "dbg_web",
            "Kronecker_21/": "kron21",
            "Kronecker_25/": "kron25",
            "Twitter/": "twit",
            "Sd1_Arc/": "web",
            "canneal_native.in": "canneal",
            "dedup_native.in": "dedup",
            "mcf_speed_inp.in": "mcf",
            "omnetpp.ini": "omnetpp",
            "t5.xml": "xalancbmk"}

vp_inputs = []
for dataset in datasets:
  vp_inputs += [dataset + "/", "DBG_" + dataset + "/"]
vp_inputs = ["Kronecker_21/"]

inputs = {
         "bfs": vp_inputs,
         "sssp": vp_inputs,
         "pagerank": vp_inputs,
         "canneal": ["canneal_native.in"],
         "dedup": ["dedup_native.in"],
         "mcf": ["mcf_speed_inp.in"],
         "omnetpp": ["omnetpp.ini"],
         "xalancbmk": ["t5.xml"]
       }

start_seed = {"Kronecker_21/": "0", "Kronecker_25/": "0", "Twitter/": "0", "Sd1_Arc/": "0", "DBG_Kronecker_25/": "3287496", "DBG_Twitter/": "15994127", "DBG_Sd1_Arc/": "18290613"}

def parse_args():
  parser = argparse.ArgumentParser()
  parser.add_argument("-a", "--app", type=str, help="Application to run (bfs, sssp, pagerank, canneal, dedup, mcf, omnetpp, xalancbmk)")
  parser.add_argument("-x", "--experiment", type=str, default=-1, help="Experiment to run (hawkeye, single_thread_pcc, sensitivity, multithread)")
  parser.add_argument("-d", "--dataset", type=str, help="Dataset to run")
  parser.add_argument("-f", "--frag_level", type=int, help="Fragmentation level (0-100%)")
  parser.add_argument("-i", "--num_iter", type=int, default=NUM_ITER, help="Number of iterations to run each experiment")
  args = parser.parse_args()
  return args

def exec_run(cmd, tmp_output, output):
  print(cmd)
  exit = os.system(cmd)
  if not exit and "screen" not in cmd:
    if tmp_output != output:
      os.system("cp -r " + tmp_output + " " + output)
      os.system("rm -r " + tmp_output)
    print("Done! Navigate to " + output + "results.txt to see the results!")
  else:
    print("Experiment failed!")

def run(exp_type, config, size=PCC_SIZE, access_time=ACCESS_TIME, num_threads=1, policy=0, demotion=False): 
  if "pcc" in exp_type:
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
    madvise = str(PROMOTION)
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
        multithread = app_dir_name + "/promotion_" + exp_type + "_" + str(size) + "_" + str(num_threads) + "_" + str(config) + "_" + str(policy)

        promotion_dir = multithread if num_threads > 1 else single_thread
        demotion_dir = multithread if num_threads > 1 else single_thread
        promotion_data = PROMOTION_CACHE_DIR + promotion_dir + "/" + dataset_names[input] + "_" + str(config)
        demotion_data = DEMOTION_CACHE_DIR + demotion_dir + "/" + dataset_names[input]

        cmd_args = ["time python measure.py", "-s", source, "-d", data, "-o", tmp_output, "-ma", madvise]
        
        cmd_args += ["-pd", promotion_data] 
        if demotion:
          cmd_args += ["-dd", demotion_data]

        if num_threads > 1:
          cmd_args += ["-t", str(num_threads)]
        
        if "bfs" in app or "sssp" in app:
          cmd_args += ["-ss", start_seed[input]]
        
        cmd_args += ["-x", str(num_iter)]
        
        cmd = " ".join(cmd_args)
        exec_run(cmd, tmp_output, exp_dir + output)

def main():
  global HOME_DIR, LAUNCH_DIR, RESULT_DIR, GRAPH_DIR, PROMOTION_CACHE_DIR, DEMOTION_CACHE_DIR
  global is_thp, apps, inputs, datasets, num_iter

  args = parse_args()

  HOME_DIR = os.path.dirname(os.getcwd()) + "/"
  LAUNCH_DIR = HOME_DIR + "applications/launch/" 
  RESULT_DIR = HOME_DIR + "results/"
  GRAPH_DIR = HOME_DIR + "data/"
  PROMOTION_CACHE_DIR = HOME_DIR + "pin3.7/source/tools/PromotionCache/output/promotion_data/"
  DEMOTION_CACHE_DIR = HOME_DIR + "pin3.7/source/tools/PromotionCache/output/demotion_data/"

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

  num_iter = args.num_iter

  if args.app:
    apps = [args.app]
    if args.dataset:
      datasets = [args.dataset]
      inputs[args.app] = [args.dataset + "/"]

  if args.experiment == "hawkeye":
    RESULT_DIR += "single_thread/"
    run("hawkeye", 0) # baseline
    if (not is_thp):
      for i in range(NUM_PERCENTAGES):
        percentage = int(math.pow(2, i))
        run("hawkeye", percentage)
      run("hawkeye", 100)

  elif args.experiment == "single_thread_pcc":
    RESULT_DIR += "single_thread/"
    run("pcc", 0) # baseline
    if (not is_thp):
      for i in range(NUM_PERCENTAGES):
        percentage = int(math.pow(2, i))
        run("pcc", percentage)
      run("pcc", 100)

  elif args.experiment == "sensitivity":
    apps = vp
    if (not is_thp):
      for i in range(NUM_SIZES):
        size = int(math.pow(2, i+2))
        for f in range(NUM_PERCENTAGES):
          run("pcc", int(math.pow(2, f)), size)
        run("pcc", 100, size)

  elif args.experiment == "multithread":
    apps = vp
    RESULT_DIR += "multithread/"
    for policy in range(NUM_POLICIES):
      for t in range(1, NUM_THREADS+1):
        num_threads = int(math.pow(2, t))
        run("pcc", 0, PCC_SIZE, num_threads, policy) # baseline
        if (not is_thp):
          for i in range(NUM_PERCENTAGES):
            percentage = int(math.pow(2, i))
            run("pcc", percentage, PCC_SIZE, num_threads, policy)
          run("pcc", 100, PCC_SIZE, num_threads, policy)

  elif args.experiment == "frag":
    apps = vp
    RESULT_DIR += "frag" + str(args.frag_level) + "/"
    run("pcc", 0, PCC_SIZE)
    if (not is_thp):
      run("hawkeye", 100, PCC_SIZE)
      run("pcc", 100, PCC_SIZE)
      run("pcc", 100, PCC_SIZE, demotion=True)
    
if __name__ == "__main__":
  main()
