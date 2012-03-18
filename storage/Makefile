include ../make_env.mk

headers=storage.h storage.include/*.h
sources=storage.c storage.include/*.c

.PHONY: all clean
all: storage.o rescuer show_file

storage.o: ${headers} ${sources}

show_file.o: show_file.c ${headers}
rescuer.o: rescuer.c ${headers}

show_file: show_file.o storage.o 
rescuer: rescuer.o storage.o

clean:
	rm -f *.o rescuer show_file

