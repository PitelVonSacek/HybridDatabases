include make_env.mk

headers= \
  database.include/*.h \
	database.types/*.h \
	utils/*.h \
	allocators/*.h \
	database.h

database_sources= \
  database.include/*.c \
	database.c

database.o: ${headers} ${database_sources} allocators/allocators.o attributes storage

.PHONY: all storage allocators attributes docs clean almost_clean
all: database.o storage allocators docs

docs: ${headers} ${database_sources}
	rm -rf doc/*
	doxygen >/dev/null


allocators/allocators.o: allocators/*
	${MAKE} -C allocators allocators.o

attributes: attributes/*
	${MAKE} -C attributes all

storage: storage/* storage/storage.include/*
	${MAKE} -C storage all

almost_clean:
	rm -f *.o
	${MAKE} -C allocators clean
	${MAKE} -C storage clean

clean:
	rm -f *.o
	${MAKE} -C attributes clean
	${MAKE} -C allocators clean
	${MAKE} -C storage clean

