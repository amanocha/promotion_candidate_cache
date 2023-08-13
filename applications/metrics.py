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
vm_haswell = ["dtlb_load_misses.walk_duration", "dtlb_store_misses.walk_duration", 
           "dtlb_load_misses.stlb_hit_4k", "dtlb_load_misses.stlb_hit_2m", 
           "dtlb_store_misses.stlb_hit_4k", "dtlb_store_misses.stlb_hit_2m", 
           "page_walker_loads.dtlb_l1", "page_walker_loads.dtlb_l2", "page_walker_loads.dtlb_l3", "page_walker_loads.dtlb_memory"]
pipeline = ["dtlb_load_misses.walk_active", "dtlb_store_misses.walk_active", "cycle_activity.stalls_l1d_miss", "cycle_activity.stalls_l2_miss", "cycle_activity.stalls_l3_miss", "cycle_activity.stalls_mem_any", "cycle_activity.stalls_total"]
metrics = general + l1cache + llc + virtual + vm_haswell
metric_vals = {}

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

    measurements.write("RESULTS:\n----------\n\n")

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