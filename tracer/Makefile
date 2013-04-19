# Define required macros here
SHELL = /bin/sh

EXENAME=fusetracer
OBJS=main.o
CFLAGS=-Wall -D_FILE_OFFSET_BITS=64
CC=g++
INCLUDES=-I/home/junhe/installs/fuse-2.9.1/include
#LIBS=`pkg-config fuse --cflags --libs` # Or place your own FUSE
                                       # library path here
LIBS+=-L/home/junhe/installs/fuse-2.9.1/lib -lfuse


${EXENAME}:${OBJS}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ ${OBJS} ${LIBS}

clean:
	-rm -f *.o core *.core *.gch
	-rm -f ${EXENAME}

.cpp.o:
	${CC} ${CFLAGS} ${INCLUDES} -c $<

run:
	./${EXENAME} /mnt/trace -d -o direct_io
