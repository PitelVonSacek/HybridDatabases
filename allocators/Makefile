CC=gcc -std=gnu99

.PHONY: all clean
all: node_allocator.o generic_allocator.o

node_allocator.o: node_allocator.c node_allocator.h 
generic_allocator: generic_allocator.c generic_allocator.h

clean:
	rm -f *.o

