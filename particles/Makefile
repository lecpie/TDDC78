CC = mpicc

CFLAGS =  -g -Wall

LFLAGS= -lpthread -lrt -lm

all: particlesc

clean:
	-$(RM) *.o particles

particlesc: main.o physics.o 
	$(CC) -o $@ main.o physics.o $(CFLAGS) $(LFLAGS)


