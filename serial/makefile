CC = cc
CFLAGS = -Wall -O3 -march=native -std=c11 -DDOUBLE_PRECISION -DNDEBUG $(USER_CFLAGS)
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