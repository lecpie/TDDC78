#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ppmio.h"
#include "blurfilter.h"
#include "gaussw.h"
#include "mpi.h"

#define MAX_RAD 1000
#define MASTER	0

int main (int argc, char ** argv)
{
    int radius;
    int xsize, ysize, colmax, np, me;
    pixel * src = malloc (MAX_PIXELS * sizeof(pixel));
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
    MPI_Bcast( &radius, 1, MPI_INT, MASTER, MPI_COMM_WORLD );
    // BROADCASTING W
    MPI_Bcast( w, MAX_RAD, MPI_DOUBLE, MASTER, MPI_COMM_WORLD );
    // BROADCASTING XSIZE
    MPI_Bcast( &xsize, 1, MPI_INT, MASTER, MPI_COMM_WORLD );
    // BROADCASTING YSIZE
    MPI_Bcast( &ysize, 1, MPI_INT, MASTER, MPI_COMM_WORLD );
    
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
    int * sendcounts, * displace;

    sendcounts = malloc(sizeof(int)*np);
    displace = malloc(sizeof(int)*np);


    int * line_starts      = malloc (np * sizeof(int)),
        * line_ends        = malloc (np * sizeof(int)),
        * line_send_starts = malloc (np * sizeof(int)),
        * line_send_ends   = malloc (np * sizeof(int)),
        * sizes            = malloc (np * sizeof(int)),
        * n_starts         = malloc (np * sizeof(int)),
        * n_sizes          = malloc (np * sizeof(int));

    int line, rem, line_sum = 0;

    int i;

    line = ysize / np;
    rem  = ysize % np;

    for (i = 0; i < np; i++) {

        sizes       [i] = (rem > i) ? line + 1: line;
        line_starts [i] = line_sum;
        line_ends   [i] = line_sum + sizes [i];
        n_starts    [i] = xsize * line_starts [i];
        n_sizes     [i] = xsize * sizes [i];

        line_send_starts [i] = ((line_starts [i] - radius) < 0)     ? 0     : line_starts [i] - radius;
        line_send_ends   [i] = ((line_ends   [i] + radius) > ysize) ? ysize : line_ends   [i] + radius;

        sendcounts [i] = (line_send_ends [i] - line_send_starts [i]) * xsize;

        displace [i] = line_send_starts [i] * xsize;

        line_sum += sizes [i];

        if (me == MASTER) {
            printf("P%d: starting from line %d to line %d (%d lines) and sending from line %d to %d (%d lines)\n",
                   i, line_starts [i],     line_ends[i],      line_ends[i]      - line_starts [i],
                      line_send_starts[i], line_send_ends[i], line_send_ends[i] - line_send_starts[i]);
        }
    }

    pixel * buffer_receiver = malloc (sizeof(pixel) * sendcounts[me]);
    
    double start, end;

    start = MPI_Wtime();
    /* DIVIDE THE IMAGE FOR THE COMPUTATION BY EACH PROCESSOR */
    MPI_Scatterv( src, sendcounts , displace, mpi_pixel_type , buffer_receiver, sendcounts[me], mpi_pixel_type , MASTER, MPI_COMM_WORLD );

    int local_ysize = line_send_ends [me] - line_send_starts [me],
        local_start = line_starts    [me] - line_send_starts [me],
        local_end   = local_start + sizes [me];

    /* COMPUTE THE BLUR FILTER */
    blurfilter_bordered(xsize, local_ysize, local_start, local_end, buffer_receiver, radius, w);

    /* GET THE PROCESSED SUBIMAGES AND MERGE THEM */
    MPI_Gatherv (buffer_receiver + local_start * xsize, n_sizes [me],
                 mpi_pixel_type, src, n_sizes, n_starts, mpi_pixel_type, MASTER , MPI_COMM_WORLD);

    end = MPI_Wtime()-start;
    
    if (me == MASTER){
		//printf("Main Master Filtering took: %g sec, s\n", (etime.tv_sec  - stime.tv_sec) + 1e-9*(etime.tv_nsec  - stime.tv_nsec)) ;
		end = MPI_Wtime()-start;
		printf("MASTER mpi time: %g sec\n",end);
		
		//print the time on the file
		FILE * fp;
		char * f;
		f = "measures.csv";
		fp = fopen(f, "a");// "w" means that we are going to write on this file, "a" appends
		fprintf(fp,"%g\n",end); // just write down the elapsed seconds: we'll make only copy&paste to the excel :)
		fclose(fp);
		
		
		/* write result */
		printf("Writing output file\n");
		if(write_ppm (argv[3], xsize, ysize,  (char *)src) != 0)
			exit(1);
	 	printf("Image written on %s\n",argv[3]);
	}

    MPI_Finalize();
    return(0);
}
