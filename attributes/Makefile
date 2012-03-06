_dummy: attributes.h attributes.inline.h

attributes.h: generate.pl defs.h
	./generate.pl 1 0 <defs.h | sed -re 's/^ #/#/' >attributes.h

attributes.inline.h: generate.pl defs.h
	./generate.pl 0 1 <defs.h | sed -re 's/^ #/#/' >attributes.inline.h

generated.h: preprocess.h definitions.h
	gcc -E preprocess.h -o /dev/stdout | grep -v '^#' | \
	  sed -re 's/[;{}]/&\n/g' > generated.h

generated_defs.h: preprocess_defs.h definitions.h
	gcc -E preprocess_defs.h -o /dev/stdout | grep -v '^#' | \
	  sed -re 's/[;{}]/&\n/g' > generated_defs.h
