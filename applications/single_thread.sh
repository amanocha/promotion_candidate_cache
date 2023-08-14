#!/bin/bash

# Run simulations for PCC promotion data 
echo "bash pin3.7/source/tools/run.sh pcc launch"
bash pin3.7/source/tools/run.sh pcc launch

# Run simulations for HawkEye promotion data
echo "bash pin3.7/source/tools/run.sh hawkeye launch"
bash pin3.7/source/tools/run.sh hawkeye launch

# Huge Page Utility
python go.py -x 2 # HawkEye
python go.py -x 3 # PCC

# Fragmented Memory
sudo bash run_frag.sh
