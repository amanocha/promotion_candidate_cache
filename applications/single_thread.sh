#!/bin/bash

# Huge Page Utility
python go.py -x 2 # HawkEye
python go.py -x 3 # PCC

# Fragmented Memory
sudo bash run_frag.sh
