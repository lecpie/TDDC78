CC=mpicc

CFLAGS = -g -Wall

LFLAGS= -lpthread -lrt -lm

all: particles

clean:
	-$(RM) *.o

particles: main.o physics.o 
	$(CC) -o $@ main.o physics.o $(CFLAGS) $(LFLAGS)


