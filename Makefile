CFLAGS=	-Wall -g -O1

lazy: lazy.o
	gcc -o $@ $<
