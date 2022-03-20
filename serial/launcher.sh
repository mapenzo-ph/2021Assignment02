#!/bin/bash

#PBS -N OMP_kd3
#PBS -q dssc
#PBS -l nodes=1:ppn=24
#PBS -l walltime=05:00:00
#PBS -o serial_kdtree.out
#PBS -j oe

cd ${PBS_O_WORKDIR}

if [ ! -d data ]
then mkdir data
fi

function run {
    make clean
    make USER_CFLAGS="-DNPTS=$1"
    echo "" > data/${1}
    for j in {1..5}
    do 
        ./serial_kdtree >> data/${1}
        echo "" >> data/${1}
    done
}

# data for alg performance
run 10000
run 20000
run 40000
run 60000
run 80000
run 100000
run 200000
run 400000
run 600000
run 800000
# run 1000000
# run 2000000
# run 4000000
# run 6000000
# run 8000000
# run 10000000
# run 20000000
# run 40000000
# run 60000000
# run 80000000
# run 100000000

