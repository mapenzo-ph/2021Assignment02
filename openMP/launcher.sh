#!/bin/bash

#PBS -N OMP_kd3
#PBS -q dssc_gpu
#PBS -l nodes=1:ppn=24
#PBS -l walltime=00:05:00
#PBS -o kdtree.out
#PBS -j oe

if [ ! -d data ]
then mkdir data
fi

cd ${PBS_O_WORKDIR}
export OMP_NUM_THREADS=8
./omp_kdtree

export OMP_NESTED=TRUE
./omp_kdtree