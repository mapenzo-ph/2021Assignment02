#!/bin/bash

#PBS -N OMP_kd3
#PBS -q dssc
#PBS -l nodes=1:ppn=24
#PBS -l walltime=05:00:00
#PBS -o omp_kdtree.out
#PBS -j oe

cd ${PBS_O_WORKDIR}

if [ ! -d data ]
then mkdir data
fi

function run {
    export OMP_NUM_THREADS=${1}
    make clean
    make USER_CFLAGS="-DNPTS=$2"
    echo > "" data/${1}_${2}
    for j in {1..5}
    do 
        ./omp_kdtree >> data/${1}_${2}
        echo "" >> data/${1}_${2}
    done
}

#data for strong scaling
# run 1 100000000
# run 2 100000000
# run 4 100000000
# run 8 100000000
# run 16 100000000

# data for weak scaling
run 1 10000000
run 2 20000000
run 4 40000000
run 8 80000000
run 16 160000000

