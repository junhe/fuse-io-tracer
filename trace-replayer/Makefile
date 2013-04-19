# Define required macros here
SHELL = /bin/sh

EXENAME=tracereplayer
OBJS=main.o
CFLAGS=-Wall -D_FILE_OFFSET_BITS=64
CC=g++
INCLUDES=
LIBS=


${EXENAME}:${OBJS}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ ${OBJS} ${LIBS}

clean:
	-rm -f *.o core *.core *.gch
	-rm -f ${EXENAME}

.cpp.o:
	${CC} ${CFLAGS} ${INCLUDES} -c $<

