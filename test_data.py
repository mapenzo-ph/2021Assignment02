"""
This script generates test data for the kdtree.
To run the script use:

$ python3 test_data.py <filename> <d> <N>

where <N> is the length of the dataset, and <d> the 
dimensionality of the data.

NOTE: import the required module before running on Orfeo

$ module load python/3.8.2/gnu/4.8.5

"""

from sys import argv
import random as r

if len(argv) != 4:
    print("""
Usage:  python3 test_data.py "/path/to/output" <NDIM> <size>  
    """)
    exit(-1)

out = open(argv[1], "w")
NDIM = int(argv[2])
size = int(argv[3])

for i in range(size):
    line = ','.join([str(r.uniform(0,100)) for i in range(NDIM)])
    print(line, file=out)

out.close()


