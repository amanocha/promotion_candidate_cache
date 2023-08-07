#include "common/common.h"

int main(int argc, char *argv[] ) {
    int run_kernel = 0;
    const char *perf_cmd, *thp_filename = "thp.txt", *pf_filename = "pf.txt", *promotion_filename = "";
    const char *other_filename = "done.txt"; 

    if (argc >= 4) run_kernel = atoi(argv[3]);
    if (argc >= 5) perf_cmd = argv[4];
    else perf_cmd = "perf stat -p %d -B -v -e dTLB-load-misses,dTLB-loads -o stat.log";
    if (argc >= 6) thp_filename = argv[5];
    if (argc >= 7) pf_filename = argv[6];
    if (argc >= 8) promotion_filename = argv[7];
    if (argc >= 9) other_filename = argv[8];
 
    // Setup
    pid = getpid();
    pidfd = syscall(SYS_pidfd_open, pid, 0);
    smaps_filename = "/proc/" + to_string(pid) + "/smaps_rollup";
    stat_filename = "/proc/" + to_string(pid) + "/stat";
    if (access(done_filename, F_OK) != -1) remove(done_filename);

    // FORK #1: Create perf process
    int cpid = fork();
    if (cpid == 0) {
        launch_perf(cpid, perf_cmd, other_filename); // child process running perf
    } else {
        setpgid(cpid, 0); // set the child the leader of its process group

        // FORK #2: Create THP tracking process
        int cpid2 = fork();
        if (cpid2 == 0) {
            launch_thp_tracking(cpid2, thp_filename, pf_filename, other_filename); // child process tracking THPs
        } else {
            setpgid(cpid2, 0); // set the child the leader of its process group

            // FORK #3: Create promotion process
            int cpid3 = fork();
            if (cpid3 == 0) {
                if (run_kernel == HAWKEYE) launch_promoter(cpid3, promotion_filename, other_filename);
            } else {
                setpgid(cpid3, 0); // set the child the leader of its process group

                // FORK #4: Create app process
                int cpid4 = fork();
                if (cpid4 == 0) { // app process
                    struct stat done_buffer, buffer;

                    while (stat (done_filename, &done_buffer) != 0 || stat (other_filename, &done_buffer) != 0) {
                        usleep(SLEEP_TIME);
                    }

                    setup_app();

                    while (stat (done_filename, &done_buffer) == 0) {
                        usleep(SLEEP_TIME);
                    }

                    cleanup_app();
                    
                    // kill child processes and all their descendants, e.g. sh, perf stat, etc.
                    kill(-cpid, SIGINT); // stop perf stat
                } else {
                    setpgid(cpid4, 0); // set the child the leader of its process group

                    cout << argv[1] << " " << argv[2] << endl;
                    int err = execl(argv[1], argv[1], argv[2], NULL);
                    if (err == -1) printf("Error with launching app!\n");

                    usleep(WAIT_TIME);
                }
            }
        }
    }

    return 0;
}
