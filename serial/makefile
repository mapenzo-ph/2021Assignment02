CC = cc
CFLAGS = -Wall -O3 -march=native -fopenmp -std=c11 -DDOUBLE_PRECISION
LDFLAGS = 

SRC = serial_kdtree.c
EXE = $(SRC:.c=)
OUT = $(SRC:.c=.out)

.PHONY:	default clean

default:	$(EXE)

%:	%.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

table:
	./scripts/clean_data.sh

plot:
	python3 scripts/plot.py

clean:
	rm $(EXE) $(OUT)