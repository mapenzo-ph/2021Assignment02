CC = mpicc
CFLAGS = -fopenmp -DDOUBLE_PRECISION
LDFLAGS = 

SRC = kdtree.c
EXE = $(SRC:.c=)
OUT = $(SRC:.c=.out)

.PHONY:	default clean

default:	$(PYOUT) $(EXE)

%:	%.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm $(EXE) $(OUT)