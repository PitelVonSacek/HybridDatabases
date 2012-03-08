CC=gcc -std=gnu99
CFLAGS+=-ggdb

database= \
  ../database.o \
  ../node_allocator.o \
  ../generic_allocator.o \
  ../storage/storage.o

generate= \
  ../generate.pl \
	../generate_node.pl \
	../generate_index.pl \
	../generate_database.pl

dictionary_test: dictionary_test.c ../utils/num_dictionary.h
	${CC} ${CFLAGS} dictionary_test.c -o dictionary_test

fast_stack_test: fast_stack_test.c ../node_allocator.o ../utils/fast_stack.h
	${CC} ${CFLAGS} fast_stack_test.c ../node_allocator.o -o fast_stack_test

node_allocator_test: node_allocator_test.c ../node_allocator.h ../node_allocator.c
	${CC} ${CFLAGS} -DNODE_ALLOCATOR_DEBUG node_allocator_test.c ../node_allocator.c -o node_allocator_test


database_test: database_test.c ${database} ${generate}
	../generate.pl "db_test_2" 1 1 s <database_test.c >database_test.preprocessed.c
	${CC} ${CFLAGS} database_test.preprocessed.c ${database} -lpthread -o database_test

.PHONY: ../% clean
../%:
	${MAKE} -C .. all

clean:
	rm -f *.o database_test database_test.preprocessed.c node_allocator_test \
	      fast_stack_test dictionary_test

