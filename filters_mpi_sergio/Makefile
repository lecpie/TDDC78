
CFLAGS = -g

LFLAGS= -g -lpthread -lrt -lm

all: blurc thresc

clean:
	-$(RM) *.o blurc thresc

blurc: ppmio.o gaussw.o blurfilter.o blurmain.o
	mpicc -o $@ ppmio.o gaussw.o blurfilter.o blurmain.o $(LFLAGS)

thresc: thresmain.o ppmio.o thresfilter.o
	mpicc   -o $@ thresmain.o ppmio.o thresfilter.o $(LFLAGS)

%.o:%.c
	mpicc -c -o $@ $^ $(CFLAGS)

arc:
	tar cf - *.c *.h *.f90 Makefile Makefile.sgi|gzip - > filters.tar.gz
