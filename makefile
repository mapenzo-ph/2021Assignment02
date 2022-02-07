CC = cc
CFLAGS = -fopenmp -std=c11 -DDOUBLE_PRECISION
LDFLAGS = 

SRC = omp_kdtree.c
EXE = $(SRC:.c=)
OUT = $(SRC:.c=.out)

.PHONY:	default clean

default:	$(PYOUT) $(EXE)

%:	%.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm $(EXE) $(OUT)