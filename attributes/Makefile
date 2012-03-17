include ../make_env.mk

.PHONY: all clean
all: attributes.h attributes.inline.h

attributes.h: generate.pl defs.h
	./generate.pl 1 0 <defs.h | sed -re 's/^ #/#/' >attributes.h

attributes.inline.h: generate.pl defs.h
	./generate.pl 0 1 <defs.h | sed -re 's/^ #/#/' >attributes.inline.h

clean:
	rm -f attributes.h attributes.inline.h

