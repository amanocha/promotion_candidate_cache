#!/bin/bash

# Run simulations for PCC promotion data
echo "sudo bash ../pin3.7/source/tools/run.sh pcc multithread"
sudo bash ../pin3.7/source/tools/run.sh pcc multithread

# Huge Page Utility
python go.py -x multithread