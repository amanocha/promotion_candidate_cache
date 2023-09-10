#!/bin/bash

# Run simulations for PCC promotion data 
echo "sudo bash ../pin3.7/source/tools/PromotionCache/run.sh pcc single_thread"
sudo bash ../pin3.7/source/tools/PromotionCache/run.sh pcc single_thread

# Run simulations for HawkEye promotion data
echo "sudo bash ../pin3.7/source/tools/PromotionCache/run.sh hawkeye single_thread"
sudo bash ../pin3.7/source/tools/PromotionCache/run.sh hawkeye single_thread

# Huge Page Utility for PCC
echo "sudo python go.py -x single_thread_pcc"
sudo python go.py -x single_thread_pcc

# Huge Page Utility for HawkEye
echo "sudo python go.py -x hawkeye"
sudo python go.py -x hawkeye

# -------------------- START THP --------------------

# Enable Linux THP
echo "echo always > /sys/kernel/mm/transparent_hugepage/enabled"
echo always > /sys/kernel/mm/transparent_hugepage/enabled

echo "echo always > /sys/kernel/mm/transparent_hugepage/defrag"
echo always > /sys/kernel/mm/transparent_hugepage/defrag

# Huge Page Utility for PCC
echo "sudo python go.py -x single_thread_pcc"
sudo python go.py -x single_thread_pcc

# Huge Page Utility for HawkEye
echo "sudo python go.py -x hawkeye"
sudo python go.py -x hawkeye

# Disable Linux THP
echo "echo madvise > /sys/kernel/mm/transparent_hugepage/enabled"
echo madvise > /sys/kernel/mm/transparent_hugepage/enabled

echo "echo madvise > /sys/kernel/mm/transparent_hugepage/defrag"
echo madvise > /sys/kernel/mm/transparent_hugepage/defrag

# -------------------- END THP --------------------
