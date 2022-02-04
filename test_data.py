from sys import argv
import random as r

out = open(argv[1], "w")
NDIM = int(argv[2])
size = int(argv[3])

for i in range(size):
    line = ','.join([str(r.uniform(0,100)) for i in range(NDIM)])
    print(line, file=out)

out.close()


