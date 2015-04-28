#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ppmio.h"
#include "blurfilter.h"
#include "gaussw.h"
#include "mpi.h"


#define MASTER	0
int main (int argc, char ** argv) {
   int radius;
    int xsize, ysize, colmax, np, me;
    pixel src[MAX_PIXELS];
    struct timespec stime, etime;
	#define MAX_RAD 1000
    double w[MAX_RAD];
    
    /* MPI INITIALIZATION */
	MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &np );
    MPI_Comm_rank( MPI_COMM_WORLD, &me );
    
    /* Take care of the arguments */
	if(me ==  MASTER){
		if (argc != 4) {
			fprintf(stderr, "Usage: %s radius infile outfile\n", argv[0]);
			exit(1);
		}
		radius = atoi(argv[1]);
		if((radius > MAX_RAD) || (radius < 1)) {
			fprintf(stderr, "Radius (%d) must be greater than zero and less then %d\n", radius, MAX_RAD);
			exit(1);
		}
		/* read file */
		if(read_ppm (argv[2], &xsize, &ysize, &colmax, (char *) src) != 0)
			exit(1);

		if (colmax > 255) {
			fprintf(stderr, "Too large maximum color-component value\n");
			exit(1);
		}
		/* filter */
		get_gauss_weights(radius, w);
		
		printf("Has read the image and generated Coefficients\n");
	}
	
	// BROADCASTING RADIUS
    MPI_Bcast( &radius, 1, MPI_INT, 0, MPI_COMM_WORLD );
    // BROADCASTING W
    MPI_Bcast( w, MAX_RAD, MPI_DOUBLE, 0, MPI_COMM_WORLD );
    // BROADCASTING XSIZE
    MPI_Bcast( &xsize, 1, MPI_INT, 0, MPI_COMM_WORLD );
     // BROADCASTING YSIZE
    MPI_Bcast( &ysize, 1, MPI_INT, 0, MPI_COMM_WORLD );
    
	/* MPI_DATATYPE : mpi_pixel_type */
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


 /* COMPUTE THE AREA TO BE EXECUTED BY EACH PROCESSOR*/
 	int * sendcounts, * displace, * recvcounts, * recvdisplace;
	sendcounts = malloc(sizeof(int)*np);  
	recvcounts = malloc(sizeof(int)*np);  
	displace = malloc(sizeof(int)*np);
	recvdisplace = malloc(sizeof(int)*np);
	int original_radius;
	int line, rem, sum;
	sum= 0;
	rem = ysize % np;
	
	int i;
	int pointer[np];
	
		line = (ysize/np);		
		original_radius = radius;
		
		for (i = 0; i < np; i++) {
			radius = original_radius;
			displace[i] = sum;
			recvdisplace[i] = displace[i];
			
			int line_rem = 0;
			if (rem > 0) {
				line_rem++;
				rem--;
			}
			
			line_rem+=line;
			recvcounts[i] = line_rem;			
			if(i == 0){
				if(displace[i]/xsize + line_rem + radius > ysize){
					radius = ysize - (displace[i]/xsize + line_rem);	
				}
				sendcounts[i] = line_rem+radius;				
				pointer[0] = 0;
			}else
			
			if(i == np-1){
				
				if(displace[i]/xsize - radius < 0){
					radius = displace[i]/xsize;
				}
				
				sendcounts[i] = line_rem + radius;
				displace[i] -= (radius*xsize);
				pointer[i] = radius*xsize;	
			
			}else
			if(i>0 && i<np-1){
				
				if(0 + displace[i]/xsize-radius < 0){
					radius = displace[i]/xsize - 0;
				}
				
				sendcounts[i] = line_rem + radius;
				int sofar = displace[i]/xsize;
				displace[i] -= (radius*xsize);
				pointer[i] = radius*xsize;

				radius = original_radius;
			
				if(sofar + line_rem + radius > ysize){
					radius = ysize - (sofar + line_rem);	
				}
				sendcounts[i] += radius;
							
			}
			
			sum += line_rem*xsize;
			sendcounts[i] *= xsize;						
			recvcounts[i] *= xsize;
			
			if(me == MASTER)
				printf("Process %d. \t Area to compute: %d . \t starting from: %d \t Displ recv: %d\n\n  ",
				 i, sendcounts[i], (displace[i]/xsize),(recvdisplace[i]/xsize));	
		}
	
	
    pixel buffer_receiver[sendcounts[me]];
    
    clock_gettime(CLOCK_REALTIME, &stime);
	
	double start, end;
	
	start = MPI_Wtime();
	/* DIVIDE THE IMAGE FOR THE COMPUTATION BY EACH PROCESSOR */ 
	MPI_Scatterv( &src, sendcounts , displace, mpi_pixel_type , &buffer_receiver, sendcounts[me], mpi_pixel_type , 0, MPI_COMM_WORLD );
	
	/* COMPUTE THE BLUR FILTER */
	blurfilter(xsize, sendcounts[me]/xsize, buffer_receiver, original_radius, w);
	
	pixel * p ;
	p= &buffer_receiver[pointer[me]];
	
	/* GET THE PROCESSED SUBIMAGES AND MERGE THEM */
	MPI_Gatherv ( p, recvcounts[me], mpi_pixel_type, &src, recvcounts, recvdisplace, mpi_pixel_type, 0 , MPI_COMM_WORLD);
	
	end = MPI_Wtime()-start;
	
	//printf("MPI time took: %g sec\n", end) ;
    clock_gettime(CLOCK_REALTIME, &etime);
    
	if(me == MASTER){
		//printf("Main Master Filtering took: %g sec, s\n", (etime.tv_sec  - stime.tv_sec) + 1e-9*(etime.tv_nsec  - stime.tv_nsec)) ;
		printf("MASTER mpi time: %g sec\n",MPI_Wtime()-start);
		/* write result */
		printf("Writing output file\n");
		if(write_ppm (argv[3], xsize, ysize,  (char *)src) != 0)
			exit(1);
	 	printf("Image written on %s\n",argv[3]);
	}

    MPI_Finalize();
    return(0);
}
