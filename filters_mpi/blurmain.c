#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <iostream>

#include <openmpi/mpi.h>

#include "ppmio.h"
#include "blurfilter.h"
#include "gaussw.h"

#define MAX_RAD 1000

#define ROOTPROC 0

using namespace std;

void die (int code)
{
    MPI_Finalize();
    exit(code);
}

int main (int argc, char ** argv) {
    MPI_Init (&argc, &argv);

    int id, np;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    double start;

    if (id == ROOTPROC) {
        int radius;
        int xsize, ysize, colmax;

        // Take care of the arguments

        if (argc != 4) {
            fprintf(stderr, "Usage: %s radius infile outfile\n", argv[0]);
            die(1);
        }

        radius = atoi(argv[1]);
        if ((radius > MAX_RAD) || (radius < 1)) {
            fprintf (stderr, "Radius (%d) must be greater than zero and less then %d\n", radius, MAX_RAD);
            die(1);
        }

        //read file
        int cur;
        if(read_ppm_head (argv[2], &xsize, &ysize, &colmax, &cur) != 0) {
            die(1);
        }

        if (colmax > 255) {
            fprintf(stderr, "Too large maximum color-component value\n");
            die(1);
        }

        start = MPI_Wtime();

        int i;

        int cur_start = cur;

        int cur_out;

        if (write_ppm_head(argv[3], xsize, ysize, &cur_out) != 0) {
            die(2);
        }

        for (i = 0; i < np; ++i) {
            MPI_Send(&xsize, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&ysize, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

            int n = ysize / np;

            if (ysize % np > i)
                ++n;

            MPI_Send(&cur_start, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&radius, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&cur_out, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

            cur_start += n * xsize * 3;
        }
    }

    int xsize, ysize, cur, n, radius, cur_out;

    MPI_Status status;

    MPI_Recv(&xsize, 1, MPI_INT, ROOTPROC, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(&ysize, 1, MPI_INT, ROOTPROC, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(&cur, 1, MPI_INT, ROOTPROC, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(&n, 1, MPI_INT, ROOTPROC, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(&radius, 1, MPI_INT, ROOTPROC, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    MPI_Recv(&cur_out, 1, MPI_INT, ROOTPROC, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    printf ("Process P%d received info data { x : %d, y : %d, cur : %d, n : %d, cur_out : %d }\n"
            , id, xsize, ysize, cur, n, cur_out);

    // repeat weights calculation in all processes for now, should be modified later
    double w[MAX_RAD];
    get_gauss_weights(radius, w);

    pixel * data = new pixel[xsize * n];

    int end;

    if (read_ppm_data(argv[2], xsize, n, cur, (char*) data, &end)) {
        printf("P%d : Error reading file\n", id);
    }

    printf ("P%d calculates horizontal blurring\n", id);
    blurfilter_x(xsize, n, data, radius, w);
    printf ("P%d ends horizontal blurring\n", id);

    int i, j;

    int ncols[np];
    int strtcol[np];
    int strtlin[np];

    int prev_col = 0;
    int prev_lin = 0;

    int beg;

    pixel * write_ptr;

    for (i = 0, beg = 0; i < np; ++i, ncols[i]) {
        MPI_Send(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

        strtlin[i] = prev_lin;
        prev_lin += ysize / np;
        if (ysize % np > i)
            ++prev_lin;

        ncols[i] = xsize / np;
        if (xsize % np > i)
            ++ncols[i];

        strtcol[i] = prev_col;
        prev_col += ncols[i];

        int length = ncols[i] * 3;

        // Sending line parts to process i for column

        for (j = 0, write_ptr = data + strtcol[i]; j < n; ++j, write_ptr += xsize) {
            MPI_Send(write_ptr, length, MPI_CHAR, i, 1, MPI_COMM_WORLD);
        }
    }

    int ncol = ncols[id];

    delete[] data;
    data = new pixel [ncol * ysize];

    int expected = np;
    int width_len = ncol * 3;

    while (expected--) {
        int lines;
        MPI_Recv(&lines, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

        for (i = 0, write_ptr = data + strtlin[status.MPI_SOURCE] * ncol; i < lines; ++i, write_ptr += ncol) {
            MPI_Recv(write_ptr, width_len, MPI_CHAR, status.MPI_SOURCE, 1, MPI_COMM_WORLD, &status);
        }
    }

    printf("P%d starts vertical bluring\n", id);
    blurfilter_y(ncol, ysize, data, radius, w);
    printf("P%d ends vertical bluring\n", id);

    // Synchronize to measure time when everyone is done
    MPI_Barrier(MPI_COMM_WORLD);

    if (id == ROOTPROC) {
        printf ("Filtering took %g seconds\n", MPI_Wtime() - start);
    }

    printf("P%d starts output\n", id, strtcol[id]);
    write_ppm_cols(argv[3], ncol, ysize, xsize, strtcol[id], cur_out, (char*) data);
    printf("P%d ends writing output\n", id);

    delete[] data;

    MPI_Finalize();

    return(0);
}
