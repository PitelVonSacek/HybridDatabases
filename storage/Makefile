HEADERS=storage.h storage.include/*.h
SOURCES=storage.c storage.include/*.c

CC=gcc -std=gnu99

storage.o: ${HEADERS} ${SOURCES}

show_file.o: show_file.c ${HEADERS}
show_file: show_file.o storage.o 
rescuer: rescuer.o storage.o

