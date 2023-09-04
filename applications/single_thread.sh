#!/bin/bash

# Run simulations for PCC promotion data 
echo "sudo bash ../pin3.7/source/tools/run.sh pcc single_thread"
#sudo bash ../pin3.7/source/tools/run.sh pcc single_thread

# Run simulations for HawkEye promotion data
echo "sudo bash ../pin3.7/source/tools/run.sh hawkeye single_thread"
#sudo bash ../pin3.7/source/tools/run.sh hawkeye single_thread

# Huge Page Utility for PCC
echo "sudo python go.py -x single_thread_pcc"
sudo python go.py -x single_thread_pcc

# Huge Page Utility for HawkEye
echo "sudo python go.py -x hawkeye"
#sudo python go.py -x hawkeye

# Fragmented Memory
echo "sudo bash run_frag.sh"
#sudo bash run_frag.sh
