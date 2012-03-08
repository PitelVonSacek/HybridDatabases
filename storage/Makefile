CC=gcc -std=gnu99

headers=storage.h storage.include/*.h
sources=storage.c storage.include/*.c

storage.o: ${headers} ${sources}

show_file.o: show_file.c ${headers}
rescuer.o: rescuer.c ${headers}

show_file: show_file.o storage.o 
rescuer: rescuer.o storage.o

all: storage.o rescuer show_file

.PHONY: clean
clean:
	rm -f *.o

