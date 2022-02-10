CC = mpicc
CFLAGS = -fopenmp -std=c11 -DDOUBLE_PRECISION
LDFLAGS = 

SRC = mpi_kdtree.c
EXE = $(SRC:.c=)
OUT = $(SRC:.c=.out)

.PHONY:	default clean

default:	$(EXE)

%:	%.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm $(EXE) $(OUT)