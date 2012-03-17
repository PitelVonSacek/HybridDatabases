CC=gcc -std=gnu99
WARNINGS=-Wall -Wuninitialized -Winit-self -Wno-switch -Wcast-align \
         -Wno-format-zero-length -Wno-parentheses
CFLAGS+= ${WARNINGS} -ggdb

headers= \
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
	utils/atomic.h \
	utils/atomic_amd64.h \
	utils/bit_array.h \
	utils/fast_stack.h \
	utils/inline_stack.h \
	utils/list.h \
	utils/num_dictionary.h \
	utils/stack.h \
	utils/static_if.h \
	utils/type_magic.h \
	utils/utils.h \
	\
	allocators/*.h \
	\
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

database.o: ${headers} ${database_sources} allocators attributes storage

.PHONY: attributes storage allocators

allocators:
	${MAKE} -C allocators all

attributes:
	${MAKE} -C attributes all

storage:
	${MAKE} -C storage all

all: database.o storage allocators

clean:
	rm -f *.o
	make -C attributes clean
	make -C allocators clean
	make -C storage clean

