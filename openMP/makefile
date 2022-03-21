CC = cc
CFLAGS = -Wall -O3 -march=native -fopenmp -std=c11 -DDOUBLE_PRECISION -DNDEBUG $(USER_CFLAGS)
LDFLAGS = 

SRC = omp_kdtree.c
EXE = $(SRC:.c=)
OUT = $(SRC:.c=.out)

.PHONY:	default clean

default:	$(EXE)

%:	%.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

tables:
	./scripts/clean_data.sh

strong:
	python3 scripts/analysis_strong.py

clean:
	rm $(EXE) $(OUT)