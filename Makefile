CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC
CINCL  = -I../mem_alloc

OBJ = data_marsh.o

all: libdata_marsh.a

data_marsh.o: data_marsh.c data_marsh.h
	$(CC) $(CFLAGS) $(CINCL) -c data_marsh.c

libdata_marsh.a: $(OBJ)
	ar rcs $@ $^

clean:
	rm -f *.o *.a
