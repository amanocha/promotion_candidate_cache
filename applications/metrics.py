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