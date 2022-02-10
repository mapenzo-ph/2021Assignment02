#!/bin/bash

#PBS -N kdtree
#PBS -q dssc
#PBS -l nodes=1:ppn=24
#PBS -l walltime=00:30:00
#PBS -o kdtree.out
#PBS -j oe

if [ ! -d data ]
then mkdir data
fi

cd ${PBS_O_WORKDIR}
