from go import *
from metrics import *

NUMA_NODE = 1
CPU = 8
CPU_LIST = [8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31]
FREQ = 2.1e9



def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-x", "--num_samples", type=int, default=5, help="Number of experiment samples")
    parser.add_argument("-s", "--source", type=str, required=True, help="Path to source code file")
    parser.add_argument("-d", "--data", type=str, required=True, help="Path to data file")
    parser.add_argument("-rp", "--relative_path", type=int, default=0, help="Use relative data file path")
    parser.add_argument("-ss", "--start_seed", type=int, default=-1, help="Start seed for BFS and SSSP")
    parser.add_argument("-t", "--num_threads", type=int, default=1, help="Number of threads")
    parser.add_argument("-o", "--output", type=str, help="Output path")
    parser.add_argument("-mo", "--measure_only", type=int, default=0, help="Perform parsing of results only (no compilation or application execution)")
    parser.add_argument("-ma", "--madvise", type=str, default="0", help="Madvise mode")
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

def execute(run_kernel, iteration=""):
    print("Executing application...")
    input_path = os.path.relpath(data, new_dir) if relative else data

    output_perf = output + "perf_output_" + run_kernel + "_" + iteration + ".txt" if iteration != "" else output + "perf_output_" + run_kernel + ".txt"
    if (os.path.isfile(output_perf)):
        return

    perf_name = "perf_output_" + run_kernel + "_" + iteration + ".txt" if iteration != "" else "perf_output_" + run_kernel + ".txt"
    app_out_name = "app_output_" + run_kernel + "_" + iteration + ".txt" if iteration != "" else "app_output_" + run_kernel + ".txt"
    err_name = "err_output_" + run_kernel + "_" + iteration + ".txt" if iteration != "" else "err_output_" + run_kernel + ".txt"
    access_name = "access_output_" + run_kernel + "_" + iteration + ".txt" if iteration != "" else "access_output_" + run_kernel + ".txt"
    pf_name = "pf_output_" + run_kernel + "_" + iteration + ".txt" if iteration != "" else "pf_output_" + run_kernel + ".txt"

    cmd_args = ["\"perf", "stat", "-p", "%d", "-B", "-v", "-o", perf_name]
    for metric in metrics:
        cmd_args += ["-e", metric + ":u"]
    perf_cmd = " ".join(cmd_args) + "\""

    start_idx = multiprocess_id if "multiprocess" in source else 0
    cpu_list = ",".join([str(t) for t in CPU_LIST[start_idx:start_idx+threads]])
    cmd_args = ["numactl", "-C", cpu_list, "--membind", str(NUMA_NODE), "sudo", "./main"] if threads > 1 else ["numactl", "-C", str(CPU_LIST[start_idx]), "--membind", str(NUMA_NODE), "sudo", "./main"]
    if app_name.replace("_multiprocess", "") not in vp:
      os.chdir(LAUNCH_DIR)
      cmd_args += ["/".join(source.split("/")[1:3])]
    cmd_args += [input_path] 
    if threads > 1:
      cmd_args += [str(threads)]
    cmd_args += [run_kernel]

    if start_seed != -1:
        cmd_args += [str(start_seed)]

    cmd_args += [perf_cmd, access_name, pf_name]

    if promotion_data != "":
        cmd_args += [promotion_data]

    if demotion_data != "":
        cmd_args += [demotion_data]

    cmd_args += [">", app_out_name, "2>", err_name]

    cmd = " ".join(cmd_args)
    print(cmd)
    os.system(cmd)
    time.sleep(10)

    for filename in [perf_name, app_out_name, err_name, access_name, pf_name]:
      os.system("sudo chmod 777 " + filename)
      os.system("mv " + filename + " " + output)

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
      match1 = re.match("\s*(\d+)\s+(\w+\.?\w*\.?\w*\.?\w*\.?\w*)", line1)

      metric1 = match1.group(2) if match1 != None else ""
      value1 = int(match1.group(1)) if match1 != None else 0

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

def main():
    global source, data, relative, measure_only, output, filename, threads
    global app_name, app_input, new_dir, start_seed, multiprocess_id, promotion_data, demotion_data

    args = parse_args()

    if not os.path.isfile(args.source):
        print("Invalid file path entered!\n")
    else:
        source = args.source
        data = args.data
        relative = args.relative_path

        filename = os.path.basename(source)
        threads = args.num_threads
        promotion_data = args.promotion_data
        demotion_data = args.demotion_data

        app_name = source.split("/")[-2]
        if (app_name.replace("_multiprocess", "") not in vp):
          app_input = os.path.basename(data).split(".")[0]
        else: #vp app
          names = data.split('/')
          app_input = names[len(names)-2] 
        new_dir = os.path.dirname(source)
        start_seed = args.start_seed

        print("Application: " + app_name)
        print("Application file path: " + source)
        print("Application input: " + app_input)

        measure_only = args.measure_only
        if args.output:
          output = args.output
        else:
          output = HOME_DIR + "results/" + app_name + "_" + app_input + "/"

        if (not os.path.isdir(output)):
          os.mkdir(output)

        print("Output directory: " + output)
        print("------------------------------------------------------------\n")
        os.chdir(new_dir)
        one = time.time()

        num_samples = args.num_samples
        run_kernel = args.madvise

        if (measure_only):
          parse_results(run_kernel)
        else:
          if "multiprocess" not in source:
            compile()
          for i in range(num_samples):
            execute(run_kernel, str(i))
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
