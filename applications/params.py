import argparse
import numpy as np
import os
import re
import sys

from go import *

# EXPERIMENT INFO
measure_only = 0
output = ""
cafe = 0

# FILE INFO
source = ""
data = ""
relative = 0

# COMPILER INFO
filename = ""
mode = ""
compile_dir = ""
flags = ""
threads = 1
tid = 0

# EXECUTION INFO
NUMA_NODE = 1
CPU = 8
CPU_LIST = [8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31]
MEM_LIB = HOME_DIR + "optimizing-huge-page-utility/utils/manage_mem.so"
EXEC = "main"
app_name = ""
app_input = ""
new_dir = ""
epoch = -1
compute_edges = -1
start_seed = 0
global_wait_output = ""
multiprocess_id = 0

hawkeye_percentage = ""
reuse_dist = ""
promotion_data = ""

# METRICS
general = ["instructions", "cpu-cycles"]
l1cache = ["L1-dcache-loads", "L1-dcache-load-misses", "L1-dcache-stores"]
l3cache = [
            "offcore_response.demand_data_rd.l3_hit.any_snoop",
            "offcore_response.demand_data_rd.l3_miss.any_snoop",
            "offcore_response.demand_data_rd.l3_miss.remote_hit_forward",
            "offcore_response.demand_data_rd.l3_miss.remote_hitm",
            "offcore_response.demand_data_rd.l3_miss_local_dram.snoop_miss_or_no_fwd",
            "offcore_response.demand_data_rd.l3_miss_remote_dram.snoop_miss_or_no_fwd",
            "offcore_response.demand_rfo.l3_hit.any_snoop",
            "offcore_response.demand_rfo.l3_miss.any_snoop",
            "offcore_response.demand_rfo.l3_miss.remote_hit_forward",
            "offcore_response.demand_rfo.l3_miss.remote_hitm",
            "offcore_response.demand_rfo.l3_miss_local_dram.snoop_miss_or_no_fwd",
            "offcore_response.demand_rfo.l3_miss_remote_dram.snoop_miss_or_no_fwd"
          ]
llc = ["LLC-loads", "LLC-load-misses", "LLC-stores", "LLC-store-misses"]
virtual = [
            "dTLB-loads", "dTLB-stores", "page-faults",
            "dtlb_load_misses.stlb_hit", "dtlb_load_misses.miss_causes_a_walk",
            "dtlb_load_misses.walk_completed", "dtlb_load_misses.walk_completed_1g", "dtlb_load_misses.walk_completed_2m_4m", "dtlb_load_misses.walk_completed_4k",
            "dtlb_store_misses.stlb_hit", "dtlb_store_misses.miss_causes_a_walk",
            "dtlb_store_misses.walk_completed", "dtlb_store_misses.walk_completed_1g", "dtlb_store_misses.walk_completed_2m_4m", "dtlb_store_misses.walk_completed_4k"
          ]
pipeline = ["dtlb_load_misses.walk_active", "dtlb_store_misses.walk_active", "cycle_activity.stalls_l1d_miss", "cycle_activity.stalls_l2_miss", "cycle_activity.stalls_l3_miss", "cycle_activity.stalls_mem_any", "cycle_activity.stalls_total"]
vm_cafe = ["dtlb_load_misses.walk_duration", "dtlb_store_misses.walk_duration", "dtlb_load_misses.stlb_hit_4k", "dtlb_load_misses.stlb_hit_2m", "dtlb_store_misses.stlb_hit_4k", "dtlb_store_misses.stlb_hit_2m", "page_walker_loads.dtlb_l1", "page_walker_loads.dtlb_l2", "page_walker_loads.dtlb_l3", "page_walker_loads.dtlb_memory"]
pipeline_cafe = ["cycle_activity.stalls_l1d_pending", "cycle_activity.stalls_l2_pending", "cycle_activity.stalls_ldm_pending", "cycle_activity.cycles_no_execute"]
metrics = general + l1cache + llc + virtual + vm_cafe
#metrics = ["cpu-cycles", "dTLB-loads", "dTLB-stores", "page-faults", 
#            "dtlb_load_misses.stlb_hit", "dtlb_load_misses.miss_causes_a_walk", "dtlb_store_misses.stlb_hit", "dtlb_store_misses.miss_causes_a_walk",
#            "dtlb_load_misses.walk_duration", "dtlb_store_misses.walk_duration", "dtlb_load_misses.walk_completed", "dtlb_store_misses.walk_completed"]
metric_vals = {}

FREQ = 2.1e9

# ---------- DIFF MEASURE ----------

def execute_diff(run_kernel, num_samples):
    print("Executing application...")
    if (relative):
        input_path = os.path.relpath(data, new_dir)
    else:
        input_path = data

    cmd_args = ["perf", "stat", "-B", "-v", "-r", num_samples,  "-o", output + "perf_output_" + run_kernel + ".txt"]
    for metric in metrics:
        cmd_args += ["-e", metric]

    cmd_args += ["./" + compile_dir + "/" + compile_dir, input_path, run_kernel]

    if start_seed:
        cmd_args += [str(start_seed)]

    if (epoch != -1 and compute_edges != -1):
        cmd_args += [str(epoch), str(compute_edges)]
    elif (epoch != -1):
        cmd_args += [str(epoch)]

    cmd_args += [">", output + "app_output_" + run_kernel + ".txt"]

    cmd = " ".join(cmd_args)
    print(cmd)
    os.system(cmd)
    #cmd1 = "export LD_LIBRARY_PATH=\"/opt/rh/llvm-toolset-7/root/usr/lib64/\"; echo $LD_LIBRARY_PATH"
    #cmd1 = cmd1 + "; " + cmd
    #print(cmd1)
    #os.system(cmd1)

def measure_diff(run_kernel):
    global metric_vals

    print("Gathering measurements...")
    with_kernel = open(output + "perf_output_" + run_kernel + ".txt", "r+")
    without_kernel = open(output + "perf_output_0.txt", "r+")

    for line1 in with_kernel:
      line1 = line1.replace(",","").replace("-", "")
      line2 = without_kernel.readline().replace(",","").replace("-", "")
      #match1 = re.match("\s*(\w+\.?\w*\.?\w*\.?\w*\.?\w*):\s+(\d+)", line1)
      #match2 = re.match("\s*(\w+\.?\w*\.?\w*\.?\w*\.?\w*):\s+(\d+)", line2)
      match1 = re.match("\s*(\d+)\s+(\w+\.?\w*\.?\w*\.?\w*\.?\w*)", line1)
      match2 = re.match("\s*(\d+)\s+(\w+\.?\w*\.?\w*\.?\w*\.?\w*)", line2)

      if match1 != None:
        metric1 = match1.group(2)
        value1 = int(match1.group(1))
      else:
        metric1 = ""
        value1 = 0

      if match2 != None:
        metric2 = match2.group(2)
        value2 = int(match2.group(1))
      else:
        metric2 = metric1
        value2 = 0

      if metric1 == metric2 and metric1 != "":
        value = value1 - value2
        if metric1 in metric_vals:
          metric_vals[metric1] += value
        else:
          metric_vals[metric1] = value
        print(metric1, value)
      '''
      if metric1 != "":
        if metric1 in metric_vals:
          metric_vals[metric1] += value1
        else:
          metric_vals[metric1] = value1
        print(metric1, value1)
      '''

    with_kernel.close()
    without_kernel.close()
    print("\n")

# ---------- DELAYED MEASURE ----------

def execute_delayed(run_kernel, iteration):
    print("Executing application...")
    if (relative):
        input_path = os.path.relpath(data, new_dir)
    else:
        input_path = data

    delay = delay_push[app_input]
    if "hybrid" in output:
        delay = delay_push_hybrid[app_input]
    elif "thp" in output:
        delay = delay_push_thp[app_input]
    elif "hp" in output:
        delay = delay_push_hp[app_input]
    if app_name in pull:
        if cafe:
             delay = str(int(delay) + int(delay_pull_cafe[app_input]))
        else:
            if "hybrid" in output:
                delay = str(int(delay) + int(delay_pull_hybrid[app_input]))
            elif "thp" in output:
                delay = str(int(delay) + int(delay_pull_thp[app_input]))
            elif "hp" in output:
                delay = str(int(delay) + int(delay_pull_hp[app_input]))
            else:
                delay = str(int(delay) + int(delay_pull[app_input]))

    cmd_args = ["perf", "stat", "-B", "-v", "-r", num_samples, "-D", delay, "-o", output + "perf_output_" + run_kernel + ".txt"]
    for metric in metrics:
        cmd_args += ["-e", metric]

    cmd_args += ["./" + compile_dir + "/" + compile_dir, input_path, run_kernel]

    if start_seed:
        cmd_args += [str(start_seed)]

    if (epoch != -1 and compute_edges != -1):
        cmd_args += [str(epoch), str(compute_edges)]
    elif (epoch != -1):
        cmd_args += [str(epoch)]

    cmd_args += [">", output + "app_output" + run_kernel + "_" + iteration + ".txt"]

    cmd = " ".join(cmd_args)
    print(cmd)
    os.system(cmd)
    #cmd1 = "export LD_LIBRARY_PATH=\"/opt/rh/llvm-toolset-7/root/usr/lib64/\"; echo $LD_LIBRARY_PATH"
    #cmd1 = cmd1 + "; " + cmd
    #print(cmd1)
    #os.system(cmd1)

# ---------- PERF RECORD ----------
def record(run_kernel, iteration):
    print("Executing application...")
    if (relative):
        input_path = os.path.relpath(data, new_dir)
    else:
        input_path = data

    cmd_args = ["perf", "record", "-o", output + "perf_output" + run_kernel + "_" + iteration + ".txt", "-F", "25750"]
    for metric in metrics:
        cmd_args += ["-e", metric]

    cmd_args += ["./" + EXEC, input_path, run_kernel]

    if start_seed:
        cmd_args += [str(start_seed)]

    if (epoch != -1 and compute_edges != -1):
        cmd_args += [str(epoch), str(compute_edges)]
    elif (epoch != -1):
        cmd_args += [str(epoch)]

    cmd_args += [">", output + "app_output" + run_kernel + "_" + iteration + ".txt"]

    cmd = " ".join(cmd_args)
    print(cmd)
    os.system(cmd)
    #cmd1 = "export LD_LIBRARY_PATH=\"/opt/rh/llvm-toolset-7/root/usr/lib64/\"; echo $LD_LIBRARY_PATH"
    #cmd1 = cmd1 + "; " + cmd
    #print(cmd1)
    #os.system(cmd1)

    cmd_args = ["perf", "report", "--stdio", "--dsos=main", "--symbols=kernel", "-i", output + "perf_output" + run_kernel + "_" + iteration + ".txt"]
    cmd_args += [">", output + "report_output" + run_kernel + "_" + iteration + ".txt"]

    cmd = " ".join(cmd_args)
    print(cmd)
    os.system(cmd)

def report(run_kernel, iteration):
    global metric_vals

    print("Gathering measurements...")
    report_file = open(output + "report_output" + run_kernel + "_" + iteration + ".txt", "r+")
    for line in report_file:
      match1 = re.match("# Samples: \d+\w*\s*of event '(.+)'", line)
      match2 = re.match("# Event count \(approx\.\): (\d+)", line)
      match3 = re.match("\s*(\d+\.\d*)%\s*main", line)
      if match1:
        metric = match1.group(1).replace("-", "")
        metric_vals[metric] = 0
      if match2:
        tot = float(match2.group(1))
      if match3:
        metric_vals[metric] = float(match3.group(1))/100*tot
    report_file.close()
    print("\n")

    for key in metric_vals:
        print(key, metric_vals[key])
