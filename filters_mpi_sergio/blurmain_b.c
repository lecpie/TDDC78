#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ppmio.h"
#include "blurfilter.h"
#include "gaussw.h"
#include "mpi.h"

#define  MASTER		0

int main (int argc, char ** argv) {
   int radius;
    int xsize, colmax;
    pixel src[MAX_PIXELS];
    struct timespec stime, etime;
    #define MAX_RAD 1000
    double w[MAX_RAD];
    MPI_Status status;
    int chunksize;
    int np, me;
    int ysize;
    MPI_Init( &argc, &argv );
    MPI_Comm_size( MPI_COMM_WORLD, &np );
    MPI_Comm_rank( MPI_COMM_WORLD, &me );

	// MASTER process reads the input arguments and the file, then distributes them to the others
    if( me == MASTER){

          /* Take care of the arguments */
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
          printf("%d Has read the image, generating coefficients\n", me);

          /* filter */
          get_gauss_weights(radius, w);
          clock_gettime(CLOCK_REALTIME, &stime);
    }

    // radius, common to everybody
    MPI_Bcast( &radius, 1, MPI_INT, 0, MPI_COMM_WORLD );

    // w, common to everybody
    MPI_Bcast( w, MAX_RAD, MPI_DOUBLE, 0, MPI_COMM_WORLD );
    //xsize, common to everybody
    MPI_Bcast( &xsize, 1, MPI_INT, 0, MPI_COMM_WORLD );
     //ysize, common to everybody
    MPI_Bcast( &ysize, 1, MPI_INT, 0, MPI_COMM_WORLD );
	

  /* create a datatype for struct pixel */
    const int nitems=3;
    int          blocklengths[3] = {1,1,1};
    MPI_Datatype types[3] = {MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR, MPI_UNSIGNED_CHAR};
    MPI_Datatype mpi_pixel_type;
    MPI_Aint     offsets[3];

    offsets[0] = offsetof(pixel, r);
    offsets[1] = offsetof(pixel, g);
	offsets[2] = offsetof(pixel, b);	
    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_pixel_type);
    MPI_Type_commit(&mpi_pixel_type);

 /* calculate areas to be computed by each processor */
    printf("Calling filter by process %d \n Parameters radius: %d , xsize: %d, ysize: %d, w: %e  received \n",me, radius, xsize, ysize, w[1]);
	int * sendcounts;
	int * displace;
	sendcounts = malloc(sizeof(int)*np);  
	displace = malloc(sizeof(int)*np);
	int line, rem, sum;
	sum= 0;
	rem = ysize % np;
	// calculate send counts and displacements
	int i;
	
	if(np>1){
		for (i = 0; i < np; i++) {
			line = ysize/np;
			if (rem > 0) {
				sendcounts[i]++;
				rem--;
			}
			if (i == 0) {
				sendcounts[i] = line + radius;
				displace[i] = sum ;
			}
			else if(i==np-1){
				sendcounts[i] = line + radius;
				displace[i] = sum -radius ;
			}else{
				sendcounts[i] = line + 2*radius;
				displace[i] = sum -radius ;
			}
			sum += line;
			if(me == MASTER)
				printf("Process %d. \t Area to compute: %d . \t Displacement: %d \n\n ", i, sendcounts[i], displace[i]);
		}
	}else{
		sendcounts[0] = ysize;
		displace[0] = 0;
	}
			
	
	
	pixel buffer_receiver[MAX_PIXELS];
    MPI_Scatterv( &src, sendcounts , displace, mpi_pixel_type , &buffer_receiver, MAX_PIXELS, mpi_pixel_type , 0, MPI_COMM_WORLD );

    blurfilter(xsize, ysize, src, radius, w);
        
	printf("\n\n #### sendcounts: %d ####\n\n", sendcounts[me]);
	MPI_Barrier( MPI_COMM_WORLD);
	if(me == 0){
		if(write_ppm (argv[3], xsize, ysize, (char *)src) != 0)
           exit(1);
	   }
	//gatherv ??

    if(me ==  MASTER){
          clock_gettime(CLOCK_REALTIME, &etime);
           printf("Filtering took: %g secs\n", (etime.tv_sec  - stime.tv_sec) +
      	    1e-9*(etime.tv_nsec  - stime.tv_nsec)) ;

          /* write result */
          //printf("Writing output file\n");
		printf("\n\nFinish\n\n");
     //     if(write_ppm (argv[3], xsize, ysize, (char *)src) != 0)
       //     exit(1);
    }
			
    MPI_Finalize();
    return(0);
}
