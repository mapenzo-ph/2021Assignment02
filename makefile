CC = mpicc
CFLAGS =

SRC = kdtree.c
EXE = $(SRC:.c=)

.PHONY:	default clean

default:	$(EXE)

%:	%.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm $(EXE)