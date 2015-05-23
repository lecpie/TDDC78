CC = mpiicc

CFLAGS =  -g -Wall -trace
CFLAGS  += -tcollect -I$(VT_ROOT)/include -L$(VT_ROOT)/lib
FFLAGS  += -tcollect -I$(VT_ROOT)/include -L$(VT_ROOT)/lib
LFLAGS  += -L$(VT_LIB_DIR) -lVT $(VT_ADD_LIBS)

LFLAGS= -lpthread -lrt -lm

all: particlesc

clean:
	-$(RM) *.o particlesc

particlesc: main.o physics.o 
	$(CC) -o $@ main.o physics.o $(CFLAGS) $(LFLAGS)


