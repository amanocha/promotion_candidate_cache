#!/bin/bash

# ----- DIRECTORIES -----
PIN_HOME=/home/aninda/pin3.7/
DIR=${PIN_HOME}source/tools/PromotionCache/
APPS_DIR=/home/aninda/optimizing-huge-page-utility/applications/
DATA_DIR=/home/aninda/graph_data/
CACHE_SIZE=128
INTERVAL=1

# ----- APPLICATIONS -----
vp_apps=(bfs sssp pagerank)
parsec_apps=(canneal dedup)
spec_apps=(mcf omnetpp xalancbmk)
other_apps=("${parsec_apps[@]}" "${spec_apps[@]}")
apps=("${vp_apps[@]}")
#apps=("${other_apps[@]}")

datasets=(Kronecker_25 Twitter Sd1_Arc DBG_Kronecker_25 DBG_Twitter DBG_Sd1_Arc) # Wikipedia DBG_Wikipedia) # DBG_Kronecker_21 DBG_Kronecker_27)
dataset_names=(kron25 twit web dbg_kron25 dbg_twit dbg_web wiki dbg_wiki)
start_seeds=(0 0 0 3287496 15994127 18290613 0 320944)
intervals=(732856447 1093269888 827860087 1379256614 1096522560 902036450 1613872676 1604682443)

other_datasets=(canneal_native.in dedup_native.in mcf_speed_inp.in omnetpp.ini t5.xml)
other_intervals=(1174268969 2602817674 981555542 1023238603)

footprints=(1 2 4 8 16 32 64 100)
cache_sizes=(4 8 16 32 64 128 256 512 1024)

exp_name=cache
#exp_name=hawkeye

run_pin() {
  filename=$1
  app_command=$2
  multithread=${3:-false}

  if [ "$multithread" = true ] ; then
    toolname="roi_mt.so"
  else
    toolname="roi.so"
  fi

  if [ ! -f $filename ]
  then
    echo "sudo ${PIN_HOME}pin -t ${DIR}obj-intel64/${toolname} -- ${app_command} > $filename"
    sudo ${PIN_HOME}pin -t ${DIR}obj-intel64/${toolname} -- ${app_command} > $filename
  fi
}

run_app() {
  filename=$1
  app_command=$2

  if [ ! -f $filename ]
  then
    echo "${app_command} > $filename"
    ${app_command} > $filename
  fi
}

parse_promotions() {
  dir=$1
  filename=$2
  cache_size=$3
  access_time=$4

  for f in ${!footprints[@]}
  do
    footprint=${footprints[$f]}
    if [ "$exp_name" == "hawkeye" ]; then
        type=${exp_name}/${access_time}_sec
    else
        type=${exp_name}_${cache_size}/${access_time}_sec
    fi
    
    if [ ! -f output/promotion_data/single_thread/${type}/${dir}/${filename}_${footprint} ]
    then
      echo "python3.6 get_promotions.py output/single_thread/${type}/$dir/$filename $footprint"
      python3.6 get_promotions.py output/single_thread/${type}/$dir/$filename $footprint
    fi
    
    if [ ! -f output/promotion_data/single_thread/${type}/${dir}/promote_all/${filename}_${footprint} ]
    then
      echo "python3.6 get_promotions.py output/single_thread/${type}/$dir/$filename $footprint True"
      python3.6 get_promotions.py output/single_thread/${type}/$dir/$filename $footprint True
    fi
  done
}

parse_demotions() {
  app=$1
  filename=$2
  cache_size=$3
  access_time=$4

  type=${exp_name}_${cache_size}/${access_time}_sec/$app
  if [ ! -f output/demotion_data/single_thread/${type}/$filename ]
  then
    echo "python3.6 get_demotions.py output/single_thread/${type}/$filename"
    python3.6 get_demotions.py output/single_thread/${type}/$filename
  fi
}

parse_promotions_mt() {
  dir=$1
  filename=$2
  cache_size=$3
  threads=$4
  policy=$5

  for f in ${!footprints[@]}
  do
    footprint=${footprints[$f]}
    type=${exp_name}_${cache_size}_${threads}
    if [ ! -f $dir/promotion_${type}_${footprint}_${policy}/$filename ]
    then
      echo "python get_promotions_mt.py output/multithread/${type}/$dir/$filename $type $footprint $policy"
      #python get_promotions_mt.py output/multithread/${type}/$dir/$filename $type $footprint $policy
    fi
  done
}

num_accesses() {
  echo "CALCULATING NUM ACCESSES"

  for a in ${!apps[@]}
  do
    app=${apps[$a]}
    echo ""

    if [[ " ${other_apps[@]} " =~ " ${app} " ]]; then
      dataset=${other_datasets[$a]}
      filename=${DIR}access_output/$app
      app_command="${APPS_DIR}launch/${app}/${app} ${DATA_DIR}${app}/${dataset}"

      run_pin $filename "$app_command"
    else
      for d in ${!datasets[@]}
      do
        dataset=${datasets[$d]}
        filename=${DIR}access_output/${dataset_names[$d]}
        start_seed=${start_seeds[$d]}
        app_command="${APPS_DIR}pin_source/${app}/main ${DATA_DIR}$dataset/ $start_seed"

        run_pin $filename "$app_command"
      done
    fi
  done
}

time_accesses() {
  echo "TIME ACCESSES"
  
  for a in ${!apps[@]}
  do
    app=${apps[$a]}
    echo ""

    if [[ " ${other_apps[@]} " =~ " ${app} " ]]; then
      dataset=${other_datasets[$a]}
      for i in {0..4}
      do
        filename=${DIR}time_output/${app}_${i}
        app_command="${APPS_DIR}launch/${app}/${app} ${DATA_DIR}${app}/${dataset}"

        run_app $filename "$app_command"
      done
    else
      for d in ${!datasets[@]}
      do
        dataset=${datasets[$d]}
        start_seed=${start_seeds[$d]}

        for i in {0..4}
        do
          filename=${DIR}time_output/${dataset_names[$d]}_${i}
          app_command="${APPS_DIR}pin_source/${app}/main ${DATA_DIR}$dataset/ $start_seed"
          
          run_app $filename "$app_command"
        done
      done
    fi
  done
}

launch() {
  echo "LAUNCH"
  size=$CACHE_SIZE
  access_time=$INTERVAL
  if [ "$exp_name" == "hawkeye" ]; then
    result_dir=${DIR}output/single_thread/${exp_name}/${access_time}_sec/
  else
    result_dir=${DIR}output/single_thread/${exp_name}_${size}/${access_time}_sec/
  fi

  for a in ${!apps[@]}
  do
    app=${apps[$a]}
    echo ""

    if [[ " ${other_apps[@]} " =~ " ${app} " ]]; then
      dataset=${other_datasets[$a]}

      if [[ ! -d ${result_dir}other ]]; then
        mkdir -p "${result_dir}other"
      fi

      filename=${result_dir}other/${app}
      access_interval=${other_intervals[$a]}

      app_command="${APPS_DIR}launch/${app}/${app} ${DATA_DIR}${app}/${dataset}"
      
      run_pin $filename "$app_command"
      parse_promotions other $app $size $access_time
    else
      for d in ${!datasets[@]}
      do
        dataset=${datasets[$d]}

        if [[ ! -d ${result_dir}${app} ]]; then
          mkdir -p "${result_dir}${app}"
        fi

        filename=${result_dir}${app}/${dataset_names[$d]}
        start_seed=${start_seeds[$d]}
        access_interval=${intervals[$d]}

        app_command="${APPS_DIR}pin_source/${app}/main ${DATA_DIR}$dataset/ $start_seed $access_interval $size $access_time"

        run_pin $filename "$app_command"
        #parse_promotions $app ${dataset_names[$d]} $size $access_time
        #parse_demotions $app ${dataset_names[$d]} $size $access_time
      done
    fi
  done
}

sensitivity() {
  echo "SENSITIVITY"
  access_time=$INTERVAL

  for a in ${!apps[@]}
  do
    app=${apps[$a]}
    echo ""

    for d in ${!datasets[@]}
    do
      dataset=${datasets[$d]}

      for s in ${!cache_sizes[@]}
      do
        cache_size=${cache_sizes[$s]}
        dir_name=cache_$cache_size
        result_dir=${DIR}output/single_thread/${dir_name}/${access_time}_sec/$app
        filename=${result_dir}/${dataset_names[$d]}
        start_seed=${start_seeds[$d]}
        access_interval=${intervals[$d]}
        
        if [ ! -d $result_dir ]
        then
          mkdir -p $result_dir
        fi

        app_command="${APPS_DIR}pin_source/$app/main ${DATA_DIR}$dataset/ $start_seed $access_interval $cache_size $access_time"

        run_pin $filename "$app_command"
        parse_promotions $app ${dataset_names[$d]} $cache_size $access_time
      done
    done 
  done
}

multithread() {
  echo "MULTITHREAD"
  size=512

  for a in ${!apps[@]}
  do
    app=${apps[$a]}
    echo ""

    if [[ " ${npb_apps[@]} " =~ " ${app} " ]]; then
      dataset=${other_datasets[$a]}
      filename=${DIR}output/multithread/${exp_name}_${size}/other/${app}
      access_interval=${other_intervals[$a]}

      app_command="${APPS_DIR}launch/${app}/${app} ${DATA_DIR}${app}/${dataset}"
      
      run_pin $filename "$app_command"
      parse_promotions_mt other $app $size
    else
      for d in ${!datasets[@]}
      do
        dataset=${datasets[$d]}

        for ((t=2; t<=16; t*=2))
        do
          dir_name=cache_${size}_${t}
          filename=${DIR}output/multithread/${dir_name}/$app/${dataset_names[$d]}
          start_seed=${start_seeds[$d]}
          access_interval=${intervals[$d]}
          
          if [ ! -d ${DIR}output/multithread/${dir_name}/$app ]
          then
            mkdir ${DIR}output/multithread/${dir_name}/$app
          fi

          app_command="${APPS_DIR}pin_source/parallel/$app/$app ${DATA_DIR}$dataset/ $start_seed $t $access_interval $size"

          run_pin $filename "$app_command" true

          for p in {0..1}
          do
            parse_promotions_mt $app ${dataset_names[$d]} $size $t $p
          done
        done
      done 
    fi
  done
}

#num_accesses
#time_accesses
launch
#sensitivity
#multithread
