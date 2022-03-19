#!/bin/bash

#PBS -N MPI_kd3
#PBS -q dssc
#PBS -l nodes=2:ppn=24
#PBS -l walltime=20:00:00
#PBS -o kdtree.out
#PBS -j oe

cd ${PBS_O_WORKDIR}
module load openmpi-4.1.1+gnu-9.3.0

if [ ! -d data ]
then mkdir data
fi
 
function run {
    cmd="mpirun -np ${1} --map-by core"
    make clean
    make USER_CFLAGS="-DNPTS=${2}"
    echo > "" data/${1}_${2}
    for j in {1..5}
    do 
        ${cmd} mpi_kdtree >> data/${1}_${2}
        echo "" >> data/${1}_${2}
    done
}

# data for strong scaling
run 1 100000000
run 2 100000000
run 4 100000000
run 8 100000000
run 16 100000000
run 32 100000000

# data for weak scaling
run 1 10000000
run 2 20000000
run 4 40000000
run 8 80000000
run 16 160000000
run 32 320000000

