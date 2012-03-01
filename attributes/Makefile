generated.h: preprocess.h definitions.h
	gcc -E preprocess.h -o /dev/stdout | grep -v '^#' | \
	  sed -re 's/[;{}]/&\n/g' > generated.h

