#!/bin/bash

DATASET=$1
APP=$2
NUMA_NODE=$3
THP_ENABLED=$4

ORDER=9
FILE="done.txt"
ITER=5

levels=(50)

for frag_level in ${levels[@]}
do
	# INITIALIZATION
	echo "INITIALIZATION"
	echo "--------------"
	
	echo "sync ; echo 3 > /proc/sys/vm/drop_caches"
	sync ; echo 3 > /proc/sys/vm/drop_caches

	echo "echo 1 > /proc/sys/vm/compact_memory"
	echo 1 > /proc/sys/vm/compact_memory

	echo "echo madvise > /sys/kernel/mm/transparent_hugepage/enabled"
	echo madvise > /sys/kernel/mm/transparent_hugepage/enabled

	echo "echo madvise > /sys/kernel/mm/transparent_hugepage/defrag"
	echo madvise > /sys/kernel/mm/transparent_hugepage/defrag

	if [ -f "$FILE" ]; then
		echo "rm $FILE"
		rm $FILE
	fi

	echo ""

	# FRAGMENT MEMORY
	echo "FRAGMENT"
	echo "--------"
	
	cmd="numactl --membind $NUMA_NODE ../utils/fragm fragment $NUMA_NODE $ORDER $frag_level"
	echo $cmd
	screen -dm -S frag $cmd
	
	pid=$(screen -ls | awk '/\.frag\t/ {print strtonum($1)}')

	echo "Waiting for fragmentation to finish..."
	while [ ! -f "$FILE" ] 
	do
		sleep 1
	done
	
	echo "Done!"

	echo ""

	# EXECUTE APP
	echo "EXECUTE APP"
	echo "-----------"

        if [ $THP_ENABLED -eq 1 ]; then
			echo "echo always > /sys/kernel/mm/transparent_hugepage/enabled"
			echo always > /sys/kernel/mm/transparent_hugepage/enabled

			echo "echo always > /sys/kernel/mm/transparent_hugepage/defrag"
			echo always > /sys/kernel/mm/transparent_hugepage/defrag
		fi

        cmd="sudo python3 go.py --experiment=frag --dataset=$DATASET --app=$APP --num_iter=$ITER"
		echo $cmd
        sleep 10
 	$cmd

	echo ""

	# CLEANUP
	echo "CLEANUP"
	echo "-------"

	echo "kill $pid"
	kill $pid

	echo ""

	cmd="../utils/free $FILE $ORDER $NUMA_NODE"
	echo $cmd
	$cmd

	echo ""

	echo "rm $FILE"
	rm $FILE

	echo ""
done