#!/bin/bash

# Run simulations for PCC promotion data 
echo "sudo bash ../pin3.7/source/tools/run.sh pcc single_thread"
sudo bash ../pin3.7/source/tools/run.sh pcc single_thread

# Run simulations for HawkEye promotion data
echo "bash ../pin3.7/source/tools/run.sh hawkeye single_thread"
bash ../pin3.7/source/tools/run.sh hawkeye single_thread

# Huge Page Utility
python go.py -x hawkeye # HawkEye
python go.py -x single_thread_pcc # PCC

# Fragmented Memory
sudo bash run_frag.sh