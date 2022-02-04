#!/bin/bash

#PBS -N kdtree
#PBS -q dssc
#PBS -l nodes=1:ppn=24
#PBS -l walltime=00:30:00
#PBS -o kdtree.out
#PBS -e kdtree.err

cd ${PBS_O_WORKDIR}
module load openmpi-4.1.1+gnu-9.3.0

mpirun -np 2 kdtree test_data.csv
