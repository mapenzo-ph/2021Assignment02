CC = cc
CFLAGS = -Wall -O3 -march=native -fopenmp -std=c11 -DNPTS=100000000 -DDOUBLE_PRECISION -DNDEBUG
LDFLAGS = 

SRC = omp_kdtree.c
EXE = $(SRC:.c=)
OUT = $(SRC:.c=.out)

.PHONY:	default clean

default:	$(EXE)

%:	%.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm $(EXE) $(OUT)