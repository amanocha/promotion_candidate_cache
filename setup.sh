#!/bin/bash

current_dir=$(dirname $(realpath -s $0))
HOME_DIR=$current_dir

NUMA_NODE=0 # EDIT THIS VALUE (NUMA NODE)

DATA_DIR=data
DATA_URL="https://decades.cs.princeton.edu/datasets/big"

datasets=(Kronecker_25 Twitter Sd1_Arc DBG_Kronecker_25 DBG_Twitter DBG_Sd1_Arc)
files=(num_nodes_edges.txt node_array.bin edge_array.bin edge_values.bin)

# DATASET PREPARATION
download_files() {
    dataset=$1

    echo -e "Downloading files for ${HOME_DIR}/${DATA_DIR}/${dataset}..."
        
    for file in "${files[@]}"
    do
        if [ ! -d ${HOME_DIR}/${DATA_DIR}/${dataset} ]
        then
            mkdir ${HOME_DIR}/${DATA_DIR}/${dataset}
        fi

        cd ${HOME_DIR}/${DATA_DIR}/${dataset}

        if [ ! -f ${HOME_DIR}/${DATA_DIR}/${dataset}/${file} ]
        then
            echo -e "wget --no-check-certificate $DATA_URL/$dataset/$file\n"
            wget --no-check-certificate $DATA_URL/$dataset/$file
        else
            echo -e "${HOME_DIR}/${DATA_DIR}/${dataset}/${file} already exists"
        fi
    done

    echo ""
}

if [ ! -d ${HOME_DIR}/${DATA_DIR} ]
then
    mkdir ${HOME_DIR}/${DATA_DIR}
fi

if cat /proc/mounts | grep -i "${HOME_DIR}/${DATA_DIR}"
then
    echo -e "${HOME_DIR}/${DATA_DIR} is mounted, checking datasets...\n"

    for dataset in "${datasets[@]}"
    do
        if [ ! -d "${HOME_DIR}/${DATA_DIR}/$dataset" ]
        then
            echo -e "$dataset is not mounted\n"
        fi

        download_files $dataset
    done
else
    echo -e "${HOME_DIR}/${DATA_DIR} is not mounted"
    
    echo "sudo mount -t tmpfs -o size=100g,mpol=bind:${NUMA_NODE} tmpfs ${HOME_DIR}/${DATA_DIR}"
    sudo mount -t tmpfs -o size=100g,mpol=bind:${NUMA_NODE} tmpfs ${HOME_DIR}/${DATA_DIR} 

    for dataset in "${datasets[@]}"
    do
        download_files $dataset
    done
fi

sudo echo 0 > /proc/sys/kernel/randomize_va_space

# WORKFLOW PREPARATION

cd ${HOME_DIR}/utils
make

cd ${HOME_DIR}
mkdir results
