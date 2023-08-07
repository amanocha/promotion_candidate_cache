#!/bin/bash

DATA_DIR=../../data
TLB_SIZE=1024

apps=(bfs sssp pagerank)

datasets=(Kronecker_25 Twitter Sd1_Arc DBG_Kronecker_25 DBG_Twitter DBG_Sd1_Arc)
datasets=(Kronecker_21)
dataset_names=(kron25 twit web dbg_kron25 dbg_twit dbg_web)
dataset_names=(kron21)
start_seed=(0 0 0 3287496 15994127 18290613)

num_nodes=(1 512 262144)
page_sizes=(4096 2097152 1073741824)
reuse_suffix=("4kb" "2mb" "1gb")

reuse_dist() {
  # Reuse Distance
  echo "REUSE DISTANCE"

  make

  for a in ${!apps[@]}
  do
    app=${apps[$a]}

    for d in ${!datasets[@]}
    do
      dataset=${datasets[$d]}
      echo "Processing $dataset:"

      if [ ! -d $app/data ]
      then
        echo "Creating $app/data"
        mkdir $app/data
      fi

      if [ ! -d $app/data/${dataset_names[$d]} ]
      then
        echo "Creating $app/data/${dataset_names[$d]}/"
        mkdir $app/data/${dataset_names[$d]}
      fi


      for (( i=0; i< ${#page_sizes[@]}; i++))
      do
        if [ ! -f $app/data/${dataset_names[$d]}/${reuse_suffix[$i]} ]
        then
          echo "./reuse ${DATA_DIR}/$dataset/ $app ${page_sizes[$i]} ${start_seed[$d]} > $app/data/${dataset_names[$d]}/${reuse_suffix[$i]}"
          ./reuse ${DATA_DIR}/$dataset/ $app ${page_sizes[$i]} ${start_seed[$d]} > $app/data/${dataset_names[$d]}/${reuse_suffix[$i]}
        fi

        if [ ! -f figs/${dataset_names[$d]}/${dataset_names[$d]}_prop_array.png ] && [ "$i" -eq $((${#page_sizes[@]} - 1)) ]
        then
          echo "python compare_reuse.py $app/data/${dataset_names[$d]} $TLB_SIZE"
          python compare_reuse.py $app/data/${dataset_names[$d]} $TLB_SIZE
        fi
      done
      echo ""
    done
  done
}

reuse_dist
