CC = mpicc
CFLAGS = -fopenmp
LDFLAGS = 

SRC = kdtree.c
EXE = $(SRC:.c=)

.PHONY:	default clean

default:	$(PYOUT) $(EXE)

%:	%.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm $(EXE)