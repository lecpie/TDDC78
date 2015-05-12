CC=ifort
CFLAGS=
OMPFLAGS=-fopenmp

SRC=laplsolv
OMP_SRC=laplsolv_omp

all : $(SRC) $(OMP_SRC)

laplsolv : $(SRC).f90
	$(CC) $(CFLAGS) -o $@ $<

laplsolv_omp : $(OMP_SRC).f90
	$(CC) $(CFLAGS) $(OMPFLAGS) -o $@ $<

clean :
	rm $(OMP_SRC) $(SRC)
