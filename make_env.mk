CC = gcc -std=gnu99
WARNINGS += -Wall -Wuninitialized -Winit-self -Wcast-align \
            -Wno-format-zero-length -Wno-parentheses

CFLAGS += ${WARNINGS} -ggdb

