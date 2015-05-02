#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ppmio.h"
#include "thresfilter.h"
#include "mpi.h"
#define MASTER	0
int main (int argc, char ** argv) {
    int xsize, ysize, colmax;
    pixel src[MAX_PIXELS];
    struct timespec stime, etime;

	int np, me;

	MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &np );
    MPI_Comm_rank( MPI_COMM_WORLD, &me );
    
    /* Take care of the arguments */
	if(me ==  MASTER){
		if (argc != 3) {
		fprintf(stderr, "Usage: %s infile outfile\n", argv[0]);
		exit(1);
		}

		/* read file */
		if(read_ppm (argv[1], &xsize, &ysize, &colmax, (char *) src) != 0)
			exit(1);

		if (colmax > 255) {
		fprintf(stderr, "Too large maximum color-component value\n");
		exit(1);
		}
	}
    printf("Has read the image, calling filter\n");


    //xsize, common to everybody
    MPI_Bcast( &xsize, 1, MPI_INT, MASTER, MPI_COMM_WORLD );
     //ysize, common to everybody
    MPI_Bcast( &ysize, 1, MPI_INT, MASTER, MPI_COMM_WORLD );
    
    
        /* create a datatype for struct pixel */
    const int nitems=3;
    int          blocklengths[3] = {1,1,1};
    MPI_Datatype types[3] = {MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR};
    MPI_Datatype mpi_pixel_type;
    MPI_Aint     offsets[3];

    offsets[0] = 0;
    offsets[1] = 1;
	offsets[2] = 2;	
    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_pixel_type);
    MPI_Type_commit(&mpi_pixel_type);

 /* calculate areas to be computed by each processor */
  //  printf("Calling filter by process %d \n Parameters radius: %d , xsize: %d, ysize: %d, w: %e  received \n",me, radius, xsize, ysize, w[1]);
	int * sendcounts;
	int * displace;

	sendcounts = malloc(sizeof(int)*np);  
	displace = malloc(sizeof(int)*np);
	int line, rem, sum;
	sum= 0;
	rem = ysize % np;
	// calculate send counts and displacements
	int i;
	
	
		line = (ysize/np);
		for (i = 0; i < np; i++) {
			displace[i] = sum;
			sendcounts[i] = 0;
			if (rem > 0) {
				sendcounts[i] = 1;
				rem--;
			}
			sendcounts[i] += line;	
			sendcounts[i] *=xsize;
		
			sum += sendcounts[i];
			if(me == MASTER)
				printf("Process %d. \t Area to compute: %d . \t Displacement send: %d \t \n\n  ",
				 i, sendcounts[i], (displace[i]/xsize));
		}
	
	
	double start, end;
	
	
    pixel buffer_receiver[sendcounts[me]];
    
	start = MPI_Wtime();
	MPI_Scatterv( &src, sendcounts , displace, mpi_pixel_type , &buffer_receiver, sendcounts[me], mpi_pixel_type , 0, MPI_COMM_WORLD );
	

    thresfilter(xsize, sendcounts[me]/xsize, buffer_receiver);

	MPI_Gatherv ( &buffer_receiver, sendcounts[me], mpi_pixel_type, &src, sendcounts, displace, mpi_pixel_type, 0 , MPI_COMM_WORLD);
	end = MPI_Wtime()-start;

	if(me== MASTER){
		printf("MASTER mpi time: %g sec\n",end);
		
		//print the time on the file
		FILE * fp;
		char * f;
		f = "measures.csv";
		fp = fopen(f, "a");// "w" means that we are going to write on this file, "a" appends
		fprintf(fp,"%g\n",end); // just write down the elapsed seconds: we'll make only copy&paste to the excel :)
		fclose(fp);
		
		printf("Writing output file\n");
		
		if(write_ppm (argv[2], xsize, ysize, (char *)src) != 0)
		  exit(1);
	}
	MPI_Finalize();

    return(0);
}
