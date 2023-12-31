from offset import *

def write_demotions(candidates):
    promoted_addr = []
    demoted_addr = []
    to_demote = []
    for time in sorted(candidates.keys()):
        stats_file.write("Time: " + str(time) + ", Demotions: " + str(len(candidates[time])) + "\n")
        for (addr, freq) in candidates[time]:
            if addr not in promoted_addr:
                promoted_addr.append(addr) # promote only once
            else:
                if freq > -1 and addr not in demoted_addr:
                    print("demoting:", addr, freq)
                    demoted_addr.append(addr)
                    to_demote.append(addr)
        to_demote.sort()
        for addr in to_demote:
            demotion_file.write(str(time) + "," + str(addr+offset) + "\n")
        stats_file.write("\n")
        to_demote = []

def process_file(filename):
    global total_num_huge_pages, MAX_DIST, MAX_FREQ

    reading = False
    candidates = {}

    if os.path.isfile(filename):
        print("READING: " + filename)
        data = open(filename)
        size = 0
        for line in data:
            mem_region_match = re.match("NODE_ARRAY: starting base = (.*), ending base = (.*)", line)
            if mem_region_match != None:
                print("MEM REGION: " + mem_region_match.group(1) + " - " + mem_region_match.group(2))
                reading = True

            if app == "sssp" or app not in vp:
                reading = True

            footprint_match = re.match("footprint: start = (\d+)KB, end = (\d+)KB, diff = (\d+)KB", line)
            if footprint_match != None:
                size = int(footprint_match.group(2))*KB_SIZE

            time_match = re.match("(\d+): Memory Regions(?: \(Cache #(\d+)\))?:", line)
            if time_match != None:
                time = int(time_match.group(1))
            
            match_str = cache_match_str if mode == "cache" else hawkeye_match_str
            match = re.match(match_str, line)
            if match != None and reading == True:
                if mode == "cache":
                    addr = int(match.group(1), 16)
                    dist = float(match.group(2))
                    freq = int(match.group(3))
                    cache_freq = int(match.group(4))
            
                    if cache_freq > 0:
                        if time not in candidates:
                            candidates[time] = []
                        candidates[time].append((addr, cache_freq))

                        if (dist > MAX_DIST):
                            MAX_DIST = dist
                        if (freq > MAX_FREQ):
                            MAX_FREQ = freq
                else:
                    addr = int(match.group(1), 16)
                    coverage = float(match.group(2))
                    bucket = int(match.group(3))

                    if time not in candidates:
                        candidates[time] = []
                    candidates[time].append(addr)

        if size == 0:
            if dataset in footprints:
                size = footprints[dataset]*KB_SIZE
            else:
                print("Footprint for " + dataset + " not found!")

        total_num_huge_pages = int((size+HUGE_PAGE_SIZE-1)/HUGE_PAGE_SIZE)
        print("Total footprint = " + str(total_num_huge_pages) + " huge pages")
        print("----------\n")
        data.close()

        write_demotions(candidates)

if __name__ == "__main__":
    filename = sys.argv[1]
    app = filename.split("/")[-2]
    dataset = filename.split("/")[-1]

    mode = "cache" if "pcc" in filename else "other"
    offset = get_offset(app, dataset, mode)

    if (len(sys.argv) > 2):
        percent = int(sys.argv[2])
        if (len(sys.argv) > 3):
            offset = int(sys.argv[3])

    DEMOTION_DIR = filename.replace("output/", "output/demotion_data/").replace(dataset, "")
    
    print("mode = " + mode + ", offset = " + str(offset) + ", demotion directory = " + DEMOTION_DIR + "\n")
    
    if (not os.path.isdir(DEMOTION_DIR)):
        os.makedirs(DEMOTION_DIR, exist_ok=True)
    demotion_filename = DEMOTION_DIR + dataset
    demotion_file = open(demotion_filename, "w+")
    stats_file = open(demotion_filename + "_stats", "w+")

    process_file(filename)

    stats_file.write("Total num of candidates: " + str(total_num_huge_pages) + "\n")
    if (mode == "cache"):
        stats_file.write("Max reuse distance: " + str(MAX_DIST) + "\nMax freq: " + str(MAX_FREQ) + "\n")

    demotion_file.close()
    stats_file.close()
