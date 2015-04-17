#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <mpi/mpi.h>

#include "ppmio.h"
#include "thresfilter.h"

#define ROOTPROC 0

void die (int ret) {
    MPI_Finalize();
    exit(ret);
}

int main (int argc, char ** argv) {
    int xsize, ysize, colmax, cur_read, lines, line_start, cur_out;

    int i;

    int id, np;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    if (id == ROOTPROC) {
        // Take care of the arguments

        if (argc != 3) {
            fprintf(stderr, "Usage: %s infile outfile\n", argv[0]);
            die(1);
        }

        if (read_ppm_head(argv[1], &xsize, &ysize, &colmax, &cur_read) != 0)
            die(1);

        if (colmax > 255) {
            fprintf(stderr, "Too large maximum color-component value\n");
            die(1);
        }

        if (write_ppm_head(argv[2], xsize, ysize, &cur_out) != 0) {
            die(2);
        }

        line_start = 0;

        for (i = 0; i < np; ++i) {

            lines = ysize / np;
            if (ysize % np > id)
                ++lines;

            MPI_Send(&xsize, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&ysize, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&cur_read, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&lines, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&line_start, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&cur_out, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

            line_start += lines;
        }

    }
    MPI_Status status;

    MPI_Recv(&xsize, 1, MPI_INT, ROOTPROC, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(&ysize, 1, MPI_INT, ROOTPROC, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(&cur_read, 1, MPI_INT, ROOTPROC, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(&lines, 1, MPI_INT, ROOTPROC, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(&line_start, 1, MPI_INT, ROOTPROC, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(&cur_out, 1, MPI_INT, ROOTPROC, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    pixel * src = new pixel[lines * xsize];

    cur_read += line_start * xsize * sizeof(pixel);
    cur_out += line_start * xsize * sizeof(pixel);

    read_ppm_data(argv[1], xsize, lines, cur_read, (char *) src, NULL);

    double start = MPI_Wtime();

    int sum, nump = xsize * lines, totalp = xsize * ysize;

    calc_sum(xsize, lines, src, nump, &sum);

    for (i = 0; i < np; ++i) {
        if (i == id) continue;

        MPI_Send(&sum, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }

    int expected = np - 1;
    int add_sum;

    while (expected--) {
        MPI_Recv(&add_sum, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

        sum += add_sum;
    }

    sum /= totalp;

    calc_thresfilter(src, nump, sum);

    MPI_Barrier(MPI_COMM_WORLD);

    if (id == ROOTPROC) {
        printf("Filtering took: %g secs\n", MPI_Wtime() - start) ;
    }

    /* write result */
    printf("P%d writing output file\n", id);
    
    if(write_ppm_lin (argv[2], xsize, lines, cur_out, (char *)src) != 0)
        die(1);

    MPI_Finalize();

    return(0);
}
