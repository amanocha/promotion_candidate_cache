#!/bin/bash

# Run simulations for PCC promotion data
echo "sudo bash ../pin3.7/source/tools/run.sh pcc multithread"
#sudo bash ../pin3.7/source/tools/run.sh pcc multithread

# Huge Page Utility
echo "sudo python go.py -x multithread"
sudo python go.py -x multithread