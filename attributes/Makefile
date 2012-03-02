_dummy: generated.h generated_defs.h

generated.h: preprocess.h definitions.h
	gcc -E preprocess.h -o /dev/stdout | grep -v '^#' | \
	  sed -re 's/[;{}]/&\n/g' > generated.h

generated_defs.h: preprocess_defs.h definitions.h
	gcc -E preprocess_defs.h -o /dev/stdout | grep -v '^#' | \
	  sed -re 's/[;{}]/&\n/g' > generated_defs.h
