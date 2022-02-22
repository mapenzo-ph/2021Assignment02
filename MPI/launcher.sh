#!/bin/bash

#PBS -N MPI_kd3
#PBS -q dssc_gpu
#PBS -l nodes=2:ppn=24
#PBS -l walltime=05:00:00
#PBS -o kdtree.out
#PBS -j oe

cd ${PBS_O_WORKDIR}
module load openmpi-4.1.1+gnu-9.3.0

if [ ! -d data ]
then mkdir data
fi

function run_kdtree_all {
    make clean
    make USER_CFLAGS="-DNPTS=$1"
    for i in {0..5}
    do
        nproc=$((2**$i))
        echo "" > data/${nproc}_${1}
        for j in {0..4}
        do  
            mpirun -np $nproc --map-by ppr:1:core --mca btl ^openib mpi_kdtree >> data/${nproc}_${1}
        done
    done
    
}

function run_kdtree_high {
    make clean
    make USER_CFLAGS="-DNPTS=$1"
    for i in {4..5}
    do
        nproc=$((2**$i))
        echo "" > data/${nproc}_${1}
        for j in {0..4}
        do  
            mpirun -np $nproc --map-by ppr:1:core --mca btl ^openib mpi_kdtree >> data/${nproc}_${1}
        done
    done
    
}

run_kdtree_all 100000
run_kdtree_all 1000000
run_kdtree_all 10000000

# do the 10e8
run_kdtree_high 100000000

# last, do same thing for 10e9 but once
make clean
npts=1000000000
make USER_CFLAGS="-DNPTS=$npts"
mpirun -np 16 --map-by ppr:1:core --mca btl ^openib mpi_kdtree >> data/16_$npts
mpirun -np 32 --map-by ppr:1:core --mca btl ^openib mpi_kdtree >> data/32_$npts