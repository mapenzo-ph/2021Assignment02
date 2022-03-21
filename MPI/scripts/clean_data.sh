#!/bin/bash

# data for strong scalability
echo "size,nprocs,time" > results/strong.csv
for file in data/*_100000000
do 
    size=$(echo $file | cut -d '/' -f 2 | cut -d '_' -f 2)
    nprocs=$(echo $file | cut -d '/' -f 2 | cut -d '_' -f 1)
    avg=$(cat $file | grep grown |
        awk '{$1=$1};{print $4}' | sed 's/s//g' |
        awk '{sum+=$1};END{print sum/NR}')
    echo $size,$nprocs,$avg >> results/strong.csv
done

echo "size,nprocs,time" > results/weak.csv
for file in $(ls data | grep -v 100000000)
do 
    size=$(echo $file | cut -d '_' -f 2)
    nprocs=$(echo $file | cut -d '_' -f 1)
    avg=$(cat data/$file | grep grown |
        awk '{$1=$1};{print $4}' | sed 's/s//g' |
        awk '{sum+=$1};END{print sum/NR}')
    echo $size,$nprocs,$avg >> results/weak.csv
done


