CC=gcc -std=gnu99
CFLAGS+= -ggdb

headers= \
  attributes/attributes.h \
  attributes/attributes.inline.h \
	\
  database.include/macros.h \
  database.include/inline.h \
  database.include/type_magic.h \
	\
	database.types/database.h \
	database.types/enums.h \
	database.types/handler.h \
	database.types/index.h \
	database.types/node.h \
  \
	storage/storage.h \
	storage/storage.include/inline.h \
	\
	utils/bitarray.h \
	utils/fast_stack.h \
	utils/inline_stack.h \
	utils/list.h \
	utils/num_dictionary.h \
	utils/stack.h \
	utils/static_if.h \
	utils/type_magic.h \
	\
	atomic.h \
	atomic_amd64.h \
	generic_allocator.h \
	node_allocator.h \
	utils.h \
	database.h

database_sources= \
  database.include/database.c \
  database.include/database_create.c \
  database.include/handler.c \
  database.include/node.c \
  database.include/read.c \
  database.include/threads.c \
  database.include/transaction.c \
  database.include/write.c \
	database.c

database.o: ${headers} ${database_sources}
node_allocator.o: node_allocator.h node_allocator.c
generic_allocator.o: generic_allocator.h generic_allocator.c

.PHONY: attributes/attributes.h attributes/attributes.inline.h \
        storage/storage.o storage

attributes/attributes.h:
	${MAKE} -C attributes attributes.h

attributes/attributes.inline.h: 
	${MAKE} -C attributes attributes.inline.h

storage/storage.o:
	${MAKE} -C storage storage.o

storage:
	${MAKE} -C storage all

all: database.o storage

clean:
	rm -f *.o
	make -C attributes clean
	make -C storage clean

