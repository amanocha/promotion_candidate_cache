from offset import *

def write_promotions(candidates):
    done_promoting = False
    unique_candidates = []
    to_promote = []
    for time in sorted(candidates.keys()):
        if done_promoting:
            break
        stats_file.write("Time: " + str(time) + ", Promotions: " + str(len(candidates[time])) + "\n")
        for addr in candidates[time]:
            if addr not in unique_candidates:
                unique_candidates.append(addr)
                to_promote.append(addr)
                if (len(unique_candidates) >= promotion_limit):
                    done_promoting = True
                    break
        to_promote.sort()
        for addr in to_promote:
            promotion_file.write(str(time) + "," + str(addr+offset) + "\n")
        stats_file.write("\n")
        to_promote = []
    if (len(unique_candidates) < promotion_limit):
      print("DID NOT PROMOTE ENOUGH PAGES: " + str(len(unique_candidates)) + "/" + str(promotion_limit))

    stats_file.write("Total huge pages promoted: " + str(promotion_limit) + "\n")

def process_file(filename):
    global total_num_huge_pages, promotion_limit, MAX_DIST, MAX_FREQ

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
            
                    if time not in candidates:
                        candidates[time] = []
                    candidates[time].append(addr)

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
        promotion_limit = int(total_num_huge_pages*percent/100)
        print("Total footprint = " + str(total_num_huge_pages) + " huge pages")
        print("----------\n")
        data.close()

        write_promotions(candidates)

if __name__ == "__main__":
    filename = sys.argv[1]
    app = filename.split("/")[-2]
    dataset = filename.split("/")[-1]

    mode = "cache" if "pcc" in filename else "other"
    offset = get_offset(app, dataset)

    if (len(sys.argv) > 2):
        percent = int(sys.argv[2])
        if (len(sys.argv) > 3):
            offset = int(sys.argv[4])

    PROMOTION_DIR = filename.replace("output/", "output/promotion_data/").replace(dataset, "")

    print("mode = " + mode + ", offset = " + str(offset) + ", promotion percent = " + str(percent) + ", promotion directory = " + PROMOTION_DIR + "\n")

    if (not os.path.isdir(PROMOTION_DIR)):
        os.makedirs(PROMOTION_DIR, exist_ok=True)
    promotion_filename = PROMOTION_DIR + dataset + "_" + str(percent)
    promotion_file = open(promotion_filename, "w+")
    stats_file = open(promotion_filename + "_stats", "w+")

    process_file(filename)

    stats_file.write("Total num of candidates: " + str(total_num_huge_pages) + "\n")
    if (mode == "cache"):
        stats_file.write("Max reuse distance: " + str(MAX_DIST) + "\nMax freq: " + str(MAX_FREQ) + "\n")

    promotion_file.close()
    stats_file.close()
