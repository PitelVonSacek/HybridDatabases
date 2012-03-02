
CC=gcc -std=gnu99
CFLAGS=-ggdb


database_headers= \
	attributes.h \
	attributes_inline.h \
	attributes_macro_hell_output.h \
	database.h \
	database_inline.h \
  database_interface.h \
	database_macros.h \
	database_node_macros.h \
	database_types.h \
	databasetype_generate.h \
	nodetype_generate.h \
	utils.h

support_headers= \
  exception.h \
	dictionary.h \
	dictionary_hash_functions.h \
	stack.h \
	storage.h \
	storage_macros.h \
	bitarray.h

storage=writer.o reader.o crc.o

storage_test: storage_test.c ${storage}
	${CC} ${CFLAGS} storage_test.c ${storage} -o storage_test

stack_test: stack_test.c stack.h
	${CC} ${CFLAGS} stack_test.c -o stack_test

dictionary_test: dictionary_test.c dictionary.c dictionary.h
	${CC} ${CFLAGS} dictionary_test.c dictionary.c -o dictionary_test

exception_test: exception_test.c exception.h exception.c
	${CC} ${CFLAGS} exception_test.c exception.c -o exception_test

exception.o: exception.c exception.h
	${CC} ${CFLAGS} -c exception.c -o exception.o


attributes_macro_hell_output.h:  attributes_macro_hell_code.h attributes_macro_hell_data.h
	${CC} -E attributes_macro_hell_code.h -o attributes_macro_hell_output_proto.h
	sed -re 's/ARGS/__VA_ARGS__/g;s/@/#/g' <attributes_macro_hell_output_proto.h >attributes_macro_hell_output.h

atributy.o: atributy.c ${database_headers} ${support_headers}
	${CC} ${CFLAFS} -c attributes.c -o attributes.o

database.o: database.c database_create.include.c threads.include.c \
            ${database_headers} ${support_headers}
	${CC} ${CFLAGS} -c database.c -o database.o

database_test: database_test.c database.o ${storage} exception.o \
               ${database_headers} ${support_headers} attributes.o dictionary.o
	${CC} ${CFLAGS} database_test.c database.o ${storage} exception.o \
	                attributes.o dictionary.o -lpthread -o database_test

show_file: show_file.c storage.h storage_macros.h ${storage} exception.h exception.o
	${CC} ${CFLAGS} show_file.c ${storage} exception.o -o show_file

rescuer: rescuer.c storage.h storage_macros.h ${storage} exception.h exception.o
	${CC} ${CFLAGS} rescuer.c ${storage} exception.o -o rescuer


${storage}: storage.h storage_macros.h writer.c reader.c crc.c utils.h

clean:
	rm *.o storage_test

