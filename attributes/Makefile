_dummy: attributes.h attributes.inline.h

attributes.h: generate.pl defs.h
	./generate.pl 1 0 <defs.h | sed -re 's/^ #/#/' >attributes.h

attributes.inline.h: generate.pl defs.h
	./generate.pl 0 1 <defs.h | sed -re 's/^ #/#/' >attributes.inline.h

.PHONY: clean

clean:
	rm -f attributes.h attributes.inline.h

