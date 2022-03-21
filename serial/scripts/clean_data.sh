#!/bin/bash
# to be launched from "serial" directory, using makefile
echo "size,time" > results/runs.csv
for file in data/*
do 
    size=$(echo $file | cut -d '/' -f 2)
    avg=$(cat $file | grep grown |
        awk '{$1=$1};{print $4}' | sed 's/s//g' |
        awk '{sum+=$1};END{print sum/NR}')
    echo $size,$avg >> results/runs.csv
done