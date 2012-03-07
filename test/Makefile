CC=gcc -std=gnu99
CFLAGS+=-ggdb

dictionary_test: dictionary_test.c ../utils/num_dictionary.h
	${CC} ${CFLAGS} dictionary_test.c -o dictionary_test

fast_stack_test: fast_stack_test.c ../node_allocator.o ../utils/fast_stack.h
	${CC} ${CFLAGS} fast_stack_test.c ../node_allocator.o -o fast_stack_test

node_allocator_test: node_allocator_test.c ../node_allocator.h ../node_allocator.c
	${CC} ${CFLAGS} -DNODE_ALLOCATOR_DEBUG node_allocator_test.c ../node_allocator.c -o node_allocator_test

database_test: database_test.c ../database.o ../node_allocator.o ../generic_allocator.o ../storage/storage.o
	${CC} ${CFLAGS} database_test.c ../database.o ../node_allocator.o \
	  ../generic_allocator.o ../storage/storage.o -lpthread -o database_test

database_test_2: database_test_2.c ../database.o ../node_allocator.o ../generic_allocator.o ../storage/storage.o
#	../generate.pl "db_test_2" 1 1 <database_test_2.c >database_test_2_.c
	${CC} ${CFLAGS} -DLOCK_DEBUG database_test_2_.c ../database.o ../node_allocator.o \
	  ../generic_allocator.o ../storage/storage.o -lpthread -o database_test



