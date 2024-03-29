#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ppmio.h"
#include "blurfilter.h"
#include "gaussw.h"

#define MAX_RAD 1000
#define NUMTHREAD 16

int main (int argc, char ** argv) {
   int radius;
    int xsize, ysize, colmax;
    pixel * src = (pixel *) malloc(sizeof(pixel) * MAX_PIXELS);
    struct timespec stime, etime;

    double w[MAX_RAD];
    unsigned nthread = NUMTHREAD;

    /* Take care of the arguments */

    if (argc != 4 && argc != 5) {
	fprintf(stderr, "Usage: %s radius infile outfile\n", argv[0]);
	exit(1);
    }
    if (argc == 5)
		nthread = atoi(argv[4]);
    
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

    printf("Has read the image, generating coefficients\n");

    /* filter */
    get_gauss_weights(radius, w);
    pixel * dst = malloc (sizeof(pixel) * xsize * ysize);

    printf("Calling filter\n");

    clock_gettime(CLOCK_REALTIME, &stime);

    blurfilter(xsize, ysize, src, dst, radius, w, nthread);

    clock_gettime(CLOCK_REALTIME, &etime);

    printf("Filtering took: %g secs\n", (etime.tv_sec  - stime.tv_sec) +
	   1e-9*(etime.tv_nsec  - stime.tv_nsec)) ;
double end = (etime.tv_sec  - stime.tv_sec) + 1e-9*(etime.tv_nsec  - stime.tv_nsec);
		//print the time on the file
		FILE * fp;
		char * f;
		f = "measures.csv";
		fp = fopen(f, "a");// "w" means that we are going to write on 
		fprintf(fp,"%g\n",end); // just write down the elapsed seconds: we'll make only copy&paste to the excel :)
		fclose(fp);

    /* write result */
    printf("Writing output file\n");
    
    if(write_ppm (argv[3], xsize, ysize, (char *)dst) != 0)
      exit(1);


    return(0);
}
