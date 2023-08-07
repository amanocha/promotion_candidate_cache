from params import *

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-x", "--num_samples", type=int, default=5, help="Number of experiment samples")
    parser.add_argument("-s", "--source", type=str, help="Path to source code file")
    parser.add_argument("-d", "--data", type=str, help="Path to data file")
    parser.add_argument("-rp", "--relative_path", type=int, default=0, help="Use relative data file path")
    parser.add_argument("-e", "--epoch", type=int, default=-1, help="Epoch used for sampling")
    parser.add_argument("-n", "--compute_edges", type=int, default=-1, help="Number of compute edges to sample with")
    parser.add_argument("-ss", "--start_seed", type=int, default=-1, help="Start seed for BFS and SSSP")
    parser.add_argument("-t", "--num_threads", type=int, default=1, help="Number of threads")
    parser.add_argument("-i", "--thread_id", type=int, default=0, help="Thread ID")
    parser.add_argument("-m", "--mode", type=str, default="db", help="Compilation variant: decades_base (db) or decades_decoupled_implicit (di) or perfect cache (PC)")
    parser.add_argument("-o", "--output", type=str, help="Output path")
    parser.add_argument("-mo", "--measure_only", type=int, default=0, help="Perform parsing of results only (no compilation or application execution)")
    parser.add_argument("-ma", "--madvise", type=str, default="1", help="Madvise mode")
    parser.add_argument("-ml", "--mlock", type=str, default="0", help="Mlock mode")
    parser.add_argument("-nd", "--num_demotions", type=str, default="0", help="Number of demotion scans")
    parser.add_argument("-hk", "--hawkeye_percentage", type=str, default="0", help="Percentage of program to consider HawkEye to guide promotions")
    parser.add_argument("-rd", "--reuse_dist", type=str, default="0", help="Use reuse distance to guide promotions")
    parser.add_argument("-pd", "--promotion_data", type=str, default="", help="Promotion filename")
    parser.add_argument("-dd", "--demotion_data", type=str, default="", help="Demotion filename")
    parser.add_argument("-mid", "--multiprocess_id", type=int, default=0, help="Multiprocess ID")
    args = parser.parse_args()
    return args

def compile():
    print("Compiling application...\n")

    if app_name in vp and threads == 1:
        cmd_args = ["g++", "-O3", "-o", "main", "-std=c++11", "-Wno-unused-result", filename]
    else:
        cmd_args = ["make"]
    cmd_args += [">", output + "compiler_output.txt", "2>", output + "compiler_err.txt"]
    cmd = " ".join(cmd_args)
    print(cmd)
    os.system(cmd)

def execute(run_kernel, num_demotions="0", mlock="0", iteration=""):
    print("Executing application...")
    if (relative):
        input_path = os.path.relpath(data, new_dir)
    else:
        input_path = data

    output_perf = output + "perf_output_" + run_kernel + "_" + iteration + ".txt" if iteration != "" else output + "perf_output_" + run_kernel + ".txt"
    if (os.path.isfile(output_perf)):
        return

    perf_name = "perf_output_" + run_kernel + "_" + iteration + ".txt" if iteration != "" else "perf_output_" + run_kernel + ".txt"
    app_out_name = "app_output_" + run_kernel + "_" + iteration + ".txt" if iteration != "" else "app_output_" + run_kernel + ".txt"
    err_name = "err_output_" + run_kernel + "_" + iteration + ".txt" if iteration != "" else "err_output_" + run_kernel + ".txt"

    cmd_args = ["\"perf", "stat", "-p", "%d", "-B", "-v", "-o", perf_name]
    for metric in metrics:
        cmd_args += ["-e", metric + ":u"]
    perf_cmd = " ".join(cmd_args) + "\""

    start_idx = multiprocess_id if "multiprocess" in source else 0
    cpu_list = ",".join([str(t) for t in CPU_LIST[start_idx:start_idx+threads]])
    cmd_args = ["numactl", "-C", cpu_list, "--membind", str(NUMA_NODE), "sudo", "./main"] if threads > 1 else ["numactl", "-C", str(CPU_LIST[start_idx]), "--membind", str(NUMA_NODE), "sudo", "./main"]
    if app_name.replace("_multiprocess", "") not in vp:
      os.chdir(LAUNCH_DIR)
      cmd_args += ["/".join(source.split("/")[1:3])] #[app_name + "/" + app_name]
    cmd_args += [input_path] 
    if threads > 1:
      cmd_args += [str(threads)]
    cmd_args += [run_kernel]

    if start_seed != -1:
        cmd_args += [str(start_seed)]

    cmd_args += [perf_cmd]

    thp_name = "thp_output_" + run_kernel + "_" + iteration + ".txt" if iteration != "" else "thp_output_" + run_kernel + ".txt"
    access_name = "access_output_" + run_kernel + "_" + iteration + ".txt" if iteration != "" else "access_output_" + run_kernel + ".txt"
    pf_name = "pf_output_" + run_kernel + "_" + iteration + ".txt" if iteration != "" else "pf_output_" + run_kernel + ".txt"
    cmd_args += [access_name, pf_name]

    if promotion_data != "":
        cmd_args += [promotion_data]

    if demotion_data != "":
        cmd_args += [demotion_data]

    if num_demotions != "0":
        cmd_args += [num_demotions]

    if mlock != "0":
        cmd_args += ["1"]

    #if (threads > 1):
    #    cmd_args += [global_wait_output + "done" + str(tid) + ".txt", global_wait_output, str(threads)]

    cmd_args += [">", app_out_name]
    cmd_args += ["2>", err_name]

    cmd = " ".join(cmd_args)
    print(cmd)
    os.system(cmd)
    time.sleep(10)

    os.system("sudo chmod 777 " + perf_name)
    os.system("sudo chmod 777 " + access_name)
    os.system("sudo chmod 777 " + pf_name)

    os.system("mv " + perf_name + " " + output)
    os.system("mv " + access_name + " " + output)
    os.system("mv " + pf_name + " " + output)
    os.system("mv " + err_name + " " + output)
    os.system("mv " + app_out_name + " " + output)

def measure(run_kernel, iteration=""):
    global metric_vals

    print("Gathering measurements...")
    filename = output + "perf_output_" + run_kernel + "_" + iteration + ".txt" if iteration != "" else output + "perf_output_" + run_kernel + ".txt"
    if (not os.path.isfile(filename)):
        return
    perf_output = open(filename, "r+")
    measurements = open(output + "measurements_" + iteration + ".txt", "w+")

    for line1 in perf_output:
      line1 = line1.replace(",","").replace("-", "")
      #match1 = re.match("\s*(\w+\.?\w*\.?\w*\.?\w*\.?\w*):\s+(\d+)", line1)
      match1 = re.match("\s*(\d+)\s+(\w+\.?\w*\.?\w*\.?\w*\.?\w*)", line1)

      if match1 != None:
        metric1 = match1.group(2)
        value1 = int(match1.group(1))
      else:
        metric1 = ""
        value1 = 0

      if metric1 != "":
        if metric1 in metric_vals:
          metric_vals[metric1] += value1
        else:
          metric_vals[metric1] = value1
        print(metric1, value1)
        measurements.write(metric1 + ": " + str(value1) + "\n")

    perf_output.close()
    measurements.close()
    print("\n")

def avg(N):
    filename = output + "measurements.txt"
    if (not os.path.isfile(filename)):
        return
    measurements = open(filename, "w+")

    measurements.write("AVERAGES:\n")
    measurements.write("---------\n")
    for metric in metric_vals:
      metric_vals[metric] = round(float(metric_vals[metric])/N, 2)
      measurements.write(metric + ": " + str(metric_vals[metric]) + "\n")

    measurements.close()

def test_parse_results(run_kernel, iteration=""):
    if iteration != "":
      results_file = "results_" + iteration + ".txt"
      measurements_file = "measurements_" + iteration + ".txt"
    else:
      results_file = "results.txt"
      measurements_file = "measurements.txt"

    measurements = open(output + results_file, "w+")

    if (measure_only):
      measure = False
      if os.path.isfile(output + measurements_file):
        averages = open(output + measurements_file, "r+")
        measure = True
      elif os.path.isfile(output + "perf_output_" + run_kernel + ".txt"):
        averages = open(output + "perf_output_" + run_kernel + ".txt", "r+")
      else:
        return
      for line in averages:
        line = line.replace(",","").replace("-", "")
        if measure:
          match = re.match("\s*(\w+\.?\w*\.?\w*\.?\w*\.?\w*):\s+(\d+\.*\d*e*\+*\d*)", line)
          if (match != None):
            metric = match.group(1)
            value = float(match.group(2))
            if metric in metric_vals:
              metric_vals[metric] += value
            else:
              metric_vals[metric] = value
            print(metric, value)
        else:
          match = re.match("\s*(\d+)\s+(\w+\.?\w*\.?\w*\.?\w*\.?\w*)", line)
          if (match != None):
            metric = match.group(2)
            value = int(match.group(1))
            if metric in metric_vals:
              metric_vals[metric] += value
            else:
              metric_vals[metric] = value
            print(metric, value)
      averages.close()

    measurements.write("RESULTS:\n")
    measurements.write("----------\n")
    measurements.write("\n")

    cycles = metric_vals["cpucycles"]

    #tlb_ld_misses = metric_vals["dTLBloadmisses"]
    #tlb_st_misses = metric_vals["dTLBstoremisses"]
    tlb_lds = metric_vals["dTLBloads"]
    tlb_sts = metric_vals["dTLBstores"]
    if tlb_sts < 0:
      tlb_sts = 0
    tlb_refs = tlb_lds + tlb_sts

    tlb_ld_stlb = metric_vals["dtlb_load_misses.stlb_hit"]
    tlb_ld_walks = metric_vals["dtlb_load_misses.miss_causes_a_walk"]
    tlb_ld_walk_cycles = metric_vals["dtlb_load_misses.walk_duration"]
    tlb_ld_walk_completed = metric_vals["dtlb_load_misses.walk_completed"]
    tlb_st_stlb = metric_vals["dtlb_store_misses.stlb_hit"]
    tlb_st_walks = metric_vals["dtlb_store_misses.miss_causes_a_walk"]
    tlb_st_walk_cycles = metric_vals["dtlb_store_misses.walk_duration"]
    tlb_st_walk_completed = metric_vals["dtlb_store_misses.walk_completed"]
    page_faults = metric_vals["pagefaults"]
    tlb_stlb = tlb_ld_stlb + tlb_st_stlb
    tlb_walks = tlb_ld_walks + tlb_st_walks
    tlb_misses = tlb_stlb + tlb_walks
    tlb_walk_cycles = tlb_ld_walk_cycles + tlb_st_walk_cycles
    tlb_walk_completed = tlb_ld_walk_completed + tlb_st_walk_completed
    tlb_miss_rate = tlb_misses*100.0/tlb_refs if (tlb_refs > 0) else 0

    measurements.write("TLB:\n")
    measurements.write("TLB Miss Rate: " + str(tlb_miss_rate) + "\n")
    if (tlb_walks > 0):
      measurements.write("STLB Miss Rate: " + str(tlb_walks*100.0/(tlb_walks+tlb_stlb)) + "\n")
      measurements.write("Page Fault Rate: " + str(page_faults*100.0/tlb_walks) + "\n")
    if (tlb_refs > 0):
      measurements.write("Percent of TLB Accesses with PT Walks: " + str(tlb_walks*100.0/tlb_refs) + "\n")
      measurements.write("Percent of TLB Accesses with Completed PT Walks: " + str(tlb_walk_completed*100.0/tlb_refs) + "\n")
      measurements.write("Percent of TLB Accesses with Page Faults: " + str(page_faults*100.0/tlb_refs) + "\n")
    measurements.write("Percent of Cycles Spent on PT Walks: " + str(tlb_walk_cycles*100.0/cycles) + "\n")
    measurements.write("Average Cycles Spent on PT Walk: " + str(tlb_walk_cycles*100.0/tlb_walk_completed) + "\n")

    measurements.close()

def parse_results(run_kernel, iteration=""):
    if iteration != "":
      results_file = "results_" + iteration + ".txt"
      measurements_file = "measurements_" + iteration + ".txt"
    else:
      results_file = "results.txt"
      measurements_file = "measurements.txt"

    measurements = open(output + results_file, "w+")

    if (measure_only):
      measure = False
      if os.path.isfile(output + measurements_file):
        averages = open(output + measurements_file, "r+")
        measure = True
      elif os.path.isfile(output + "perf_output_" + run_kernel + ".txt"):
        averages = open(output + "perf_output_" + run_kernel + ".txt", "r+")
      else:
        return
      for line in averages:
        line = line.replace(",","").replace("-", "")
        if measure:
          match = re.match("\s*(\w+\.?\w*\.?\w*\.?\w*\.?\w*):\s+(\d+\.*\d*e*\+*\d*)", line)
          if (match != None):
            metric = match.group(1)
            value = float(match.group(2))
            if metric in metric_vals:
              metric_vals[metric] += value
            else:
              metric_vals[metric] = value
            print(metric, value)
        else:
          match = re.match("\s*(\d+)\s+(\w+\.?\w*\.?\w*\.?\w*\.?\w*)", line)
          if (match != None):
            metric = match.group(2)
            value = int(match.group(1))
            if metric in metric_vals:
              metric_vals[metric] += value
            else:
              metric_vals[metric] = value
            print(metric, value)
      averages.close()

    measurements.write("RESULTS:\n")
    measurements.write("----------\n")
    measurements.write("\n")

    cycles = metric_vals["cpucycles"]

    l1_ld_misses = metric_vals["L1dcacheloadmisses"]
    l1_lds = metric_vals["L1dcacheloads"]
    l1_st_misses = 0 #metric_vals["L1dcachestoremisses"]
    l1_sts = metric_vals["L1dcachestores"]
    l1_misses = l1_ld_misses + l1_st_misses
    l1_refs = l1_lds + l1_sts

    llc_ld_misses = metric_vals["LLCloadmisses"]
    llc_lds = metric_vals["LLCloads"]
    llc_st_misses = metric_vals["LLCstoremisses"]
    llc_sts = metric_vals["LLCstores"]
    llc_misses = llc_ld_misses + llc_st_misses
    llc_refs = llc_lds + llc_sts

    measurements.write("CACHE:\n")
    measurements.write("L1 Miss Rate: " + str(l1_misses*100.0/l1_refs) + "\n")
    measurements.write("LLC Miss Rate: " + str(llc_misses*100.0/llc_refs) + "\n")
    measurements.write("L3 Miss: " + str(llc_misses*100.0/l1_refs) + "\n")

    measurements.write("\n")

    #tlb_ld_misses = metric_vals["dTLBloadmisses"]
    tlb_lds = metric_vals["dTLBloads"]
    #tlb_st_misses = metric_vals["dTLBstoremisses"]
    if metric_vals["dTLBstores"] > 0:
      tlb_sts = metric_vals["dTLBstores"]
    else:
      tlb_sts = 0
    tlb_refs = tlb_lds + tlb_sts
    #tlb_misses = tlb_ld_misses + tlb_st_misses

    tlb_ld_stlb = metric_vals["dtlb_load_misses.stlb_hit"]
    tlb_ld_walks = metric_vals["dtlb_load_misses.miss_causes_a_walk"]
    tlb_ld_walk_cycles = metric_vals["dtlb_load_misses.walk_duration"]
    tlb_ld_walk_completed = metric_vals["dtlb_load_misses.walk_completed"]
    tlb_ld_walk_completed_4k = metric_vals["dtlb_load_misses.walk_completed_4k"]
    tlb_ld_walk_completed_2m = metric_vals["dtlb_load_misses.walk_completed_2m_4m"]
    tlb_ld_walk_completed_1g = metric_vals["dtlb_load_misses.walk_completed_1g"]
    tlb_st_stlb = metric_vals["dtlb_store_misses.stlb_hit"]
    tlb_st_walks = metric_vals["dtlb_store_misses.miss_causes_a_walk"]
    tlb_st_walk_cycles = metric_vals["dtlb_store_misses.walk_duration"]
    tlb_st_walk_completed = metric_vals["dtlb_store_misses.walk_completed"]
    tlb_st_walk_completed_4k = metric_vals["dtlb_store_misses.walk_completed_4k"]
    tlb_st_walk_completed_2m = metric_vals["dtlb_store_misses.walk_completed_2m_4m"]
    tlb_st_walk_completed_1g = metric_vals["dtlb_store_misses.walk_completed_1g"]
    page_faults = metric_vals["pagefaults"]
    tlb_stlb = tlb_ld_stlb + tlb_st_stlb
    tlb_walks = tlb_ld_walks + tlb_st_walks
    tlb_misses = tlb_stlb + tlb_walks
    tlb_walk_cycles = tlb_ld_walk_cycles + tlb_st_walk_cycles
    tlb_walk_completed = tlb_ld_walk_completed + tlb_st_walk_completed
    tlb_walk_completed_4k = tlb_ld_walk_completed_4k + tlb_st_walk_completed_4k
    tlb_walk_completed_2m = tlb_ld_walk_completed_2m + tlb_st_walk_completed_2m
    tlb_walk_completed_1g = tlb_ld_walk_completed_1g + tlb_st_walk_completed_1g
    tlb_walk_completed_tot = tlb_walk_completed_4k + tlb_walk_completed_2m + tlb_walk_completed_1g
    tlb_miss_rate = tlb_misses*100.0/tlb_refs if (tlb_refs > 0) else 0

    tlb_ld_stlb_4k = metric_vals["dtlb_load_misses.stlb_hit_4k"]
    tlb_ld_stlb_2m = metric_vals["dtlb_load_misses.stlb_hit_2m"]
    tlb_st_stlb_4k = metric_vals["dtlb_store_misses.stlb_hit_4k"]
    tlb_st_stlb_2m = metric_vals["dtlb_store_misses.stlb_hit_2m"]
    tlb_stlb_4k = tlb_ld_stlb_4k + tlb_st_stlb_4k
    tlb_stlb_2m = tlb_ld_stlb_2m + tlb_st_stlb_2m
    walker_loads_l1 = metric_vals["page_walker_loads.dtlb_l1"]
    walker_loads_l2 = metric_vals["page_walker_loads.dtlb_l2"]
    walker_loads_l3 = metric_vals["page_walker_loads.dtlb_l3"]
    walker_loads_mem = metric_vals["page_walker_loads.dtlb_memory"]
    measurements.write("4KB STLB Hit: " + str(tlb_stlb_4k*100.0/tlb_stlb) + "\n")
    measurements.write("2MB STLB Hit: " + str(tlb_stlb_2m*100.0/tlb_stlb) + "\n")
    measurements.write("Page Walker L1 Loads: " + str(walker_loads_l1*100.0/tlb_walks) + "\n")
    measurements.write("Page Walker L2 Loads: " + str(walker_loads_l2*100.0/tlb_walks) + "\n")
    measurements.write("Page Walker L3 Loads: " + str(walker_loads_l3*100.0/tlb_walks) + "\n")
    measurements.write("Page Walker Mem Loads: " + str(walker_loads_mem*100.0/tlb_walks) + "\n")
    measurements.write("\n")

    measurements.write("TLB:\n")
    measurements.write("TLB Miss Rate: " + str(tlb_miss_rate) + "\n")
    if (tlb_walks > 0):
      measurements.write("STLB Miss Rate: " + str(tlb_walks*100.0/(tlb_walks+tlb_stlb)) + "\n")
      measurements.write("Page Fault Rate: " + str(page_faults*100.0/tlb_walks) + "\n")
    if (tlb_refs > 0):
      measurements.write("Percent of TLB Accesses with PT Walks: " + str(tlb_walks*100.0/tlb_refs) + "\n")
      measurements.write("Percent of TLB Accesses with Completed PT Walks: " + str(tlb_walk_completed*100.0/tlb_refs) + "\n")
      measurements.write("Percent of TLB Accesses with Page Faults: " + str(page_faults*100.0/tlb_refs) + "\n")
    measurements.write("Percent of Cycles Spent on PT Walks: " + str(tlb_walk_cycles*100.0/cycles) + "\n")
    measurements.write("Average Cycles Spent on PT Walk: " + str(tlb_walk_cycles*100.0/tlb_walk_completed) + "\n")
    if (tlb_walk_completed_tot > 0):
      measurements.write("4KB Page Table Walks: " + str(tlb_walk_completed_4k*100.0/tlb_walk_completed_tot) + "\n")
      measurements.write("2MB/4MB Page Table Walks: " + str(tlb_walk_completed_2m*100.0/tlb_walk_completed_tot) + "\n")
      measurements.write("1GB Page Table Walks: " + str(tlb_walk_completed_1g*100.0/tlb_walk_completed_tot) + "\n")
    measurements.write("\n")

    l1_stalls = metric_vals["cycle_activity.stalls_l1d_pending"] if "cycle_activity.stalls_l1d_pending" in metric_vals else 0
    l2_stalls = metric_vals["cycle_activity.stalls_l2_pending"] if "cycle_activity.stalls_l2_pending" in metric_vals else 0
    l3_stalls = 0
    mem_stalls = metric_vals["cycle_activity.stalls_ldm_pending"] if "cycle_activity.stalls_ldm_pending" in metric_vals else 0
    tot_stalls = metric_vals["cycle_activity.cycles_no_execute"] if "cycle_activity.cycles_no_execute" in metric_vals else 0

    instructions = metric_vals["instructions"]
    memory_ops = l1_refs
    compute_ops = instructions - memory_ops
    ratio = compute_ops/memory_ops
    avg_bw = llc_misses*64*4/(1024*1024*1024)/(cycles/FREQ)
    ipc = float(instructions)/cycles

    measurements.write("PIPELINE:\n")
    measurements.write("Cycles: " + str(cycles) + "\n")
    if (cycles > 0):
      measurements.write("Percent of Cycles Spent on L1 Miss Stalls: " + str(l1_stalls*100.0/cycles) + "\n")
      measurements.write("Percent of Cycles Spent on L2 Miss Stalls: " + str(l2_stalls*100.0/cycles) + "\n")
      measurements.write("Percent of Cycles Spent on L3 Miss Stalls: " + str(l3_stalls*100.0/cycles) + "\n")
      measurements.write("Percent of Cycles Spent on Mem Subsystem Stalls: " + str(mem_stalls*100.0/cycles) + "\n")
      measurements.write("Percent of Cycles Spent on All Stalls: " + str(tot_stalls*100.0/cycles) + "\n")
      measurements.write("Calculated IPC: " + str(ipc) + "\n")
    measurements.write("Calculated Compute to Memory Ratio: " + str(ratio) + "\n")
    measurements.write("Average BW: " + str(avg_bw) + "\n")
    measurements.write("Total Number of Memory Instructions: " + str(round((tlb_lds+tlb_sts),3)) + "\n")
    measurements.write("\n")
    measurements.write("Percent of Instructions Spent on Memory: " + str(round((tlb_lds+tlb_sts)*100.0/instructions,3)) + "\n")
    measurements.write("Percent of Instructions Spent on Loads: " + str(round(tlb_lds*100.0/instructions,3)) + "\n")
    measurements.write("Percent of Instructions Spent on Stores: " + str(round(tlb_sts*100.0/instructions,3)) + "\n")
    measurements.write("Percent of Memory Instructions Spent on Loads: " + str(round(tlb_lds*100.0/(tlb_lds+tlb_sts),3)) + "\n")
    measurements.write("Percent of Memory Instructions Spent on Stores: " + str(round(tlb_sts*100.0/(tlb_lds+tlb_sts),3)) + "\n")
    measurements.write("\n")

    measurements.close()

def main():
    global source, data, relative, measure_only, output, metrics
    global filename, mode, compile_dir, flags, threads, tid
    global app_name, app_input, new_dir, epoch, compute_edges, start_seed, global_wait_output, multiprocess_id
    global hawkeye_percentage, reuse_dist, promotion_data, demotion_data

    args = parse_args()

    if not os.path.isfile(args.source):
        print("Invalid file path entered!\n")
    else:
        source = args.source
        data = args.data
        relative = args.relative_path

        #metrics += vm_cafe
        #metrics += pipeline_cafe

        filename = os.path.basename(source)
        mode = args.mode
        threads = args.num_threads
        tid = args.thread_id
        hawkeye_percentage = args.hawkeye_percentage
        reuse_dist = args.reuse_dist
        promotion_data = args.promotion_data
        demotion_data = args.demotion_data

        if mode == "di":
            compile_dir = "decades_decoupled_implicit"
            flags = flags + "-d"
        else:
            compile_dir = "decades_base"

        app_name = source.split("/")[-2]
        if (app_name.replace("_multiprocess", "") not in vp):
          app_input = os.path.basename(data).split(".")[0]
        else: #vp app
          names = data.split('/')
          app_input = names[len(names)-2] 
        new_dir = os.path.dirname(source)
        epoch = args.epoch
        compute_edges = args.compute_edges
        start_seed = args.start_seed

        print("Application: " + app_name)
        print("Application file path: " + source)
        print("Application input: " + app_input)

        measure_only = args.measure_only
        if args.output:
          output = args.output
        else:
          output = "/home/amanocha/graph_apps/results/" + app_name + "_" + app_input + "/"

        if (not os.path.isdir(output)):
          os.mkdir(output)

        print("Output directory: " + output)
        print("------------------------------------------------------------\n")
        os.chdir(new_dir)
        one = time.time()

        num_samples = args.num_samples
        run_kernel = args.madvise
        num_demotions = args.num_demotions
        mlock = args.mlock

        if (measure_only):
          parse_results(run_kernel)
        else:
          if "multiprocess" not in source:
            compile()
          for i in range(num_samples):
            execute(run_kernel, num_demotions, mlock, str(i))
            measure(run_kernel, str(i))
            parse_results(run_kernel, str(i))
          avg(num_samples)
          parse_results(run_kernel)
        two = time.time()
        print("Total Time = " + str(round(two - one)) + " seconds.\n")

        if "multiprocess" in source:
          new_output = output.replace("tmp_", "")
          os.system("mv " + output + " " + new_output)
          print("Done! Navigate to " + new_output + "results.txt to see the results!")

if __name__ == "__main__":
    main()
