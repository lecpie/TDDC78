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


    pixel * data;

    if ((data = (pixel *) malloc(xsize * n * 3)) == NULL)
        perror("malloc");

    printf("P%d has data array of size %d\n", id, xsize * n * 3);

    int end;

    printf ("P%d read %d bytes in file from %d\n", id, n * xsize, cur);
    if (read_ppm_data(argv[2], xsize, n, cur, (char*) data, &end)) {
        printf("P%d : Error reading file\n", id);
    }
    printf ("P%d ended reading at %d\n", id, end);

    printf ("P%d calculates horizontal blurring\n", id);
    blurfilter_x(xsize, n, data, radius, w);
    printf ("P%d ends horizontal blurring\n", id);

    int i, j;

    int ncols[np];
    int strtcol[np];
    int prev = 0;

    int strtlin[np];
    int prev_lin = 0;

    int beg = 0;


    for (i = 0; i < np; ++i) {
        MPI_Send(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

        strtlin[i] = prev_lin;
        prev_lin += ysize / np;
        if (ysize % np > i)
            ++prev_lin;

        int ncol = xsize / np;
        if (xsize % np > i)
            ++ncol;
        ncols[i] = ncol;
        strtcol[i] = prev;
        prev += ncol;

        printf("P%d sends %d lines of size %d to %d from column %d\n", id, n, ncols[i], i, beg);
        for (j = 0; j < n; ++j) {

            int off = j * xsize + strtcol[i];

            MPI_Send(data + off, ncols[i] * 3, MPI_CHAR, i, 1, MPI_COMM_WORLD);
        }

        beg += ncols[i];
    }

    int ncol = ncols[id];


    delete[] data;
    data = new pixel [ncol * ysize];

    int expected = np;
    while (expected--) {
        printf("P%d awaits\n", id);
        int lines;
        MPI_Recv(&lines, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

        printf("P%d receiving %d lines from P%d\n", id, lines, status.MPI_SOURCE);

        for (i = 0; i < lines; ++i) {
            int off = (strtlin[status.MPI_SOURCE] + i) * ncol;
            MPI_Recv(data + off, ncol * 3, MPI_CHAR, status.MPI_SOURCE, 1, MPI_COMM_WORLD, &status);
        }
        printf("P%d finished receiving from P%d\n", id, status.MPI_SOURCE);
    }

    printf("P%d starts vertical bluring\n", id);
    blurfilter_y(ncol, ysize, data, radius, w);
    printf("P%d ends vertical bluring\n", id);


    printf("P%d starts output col %d to %d, %d lines\n", id, strtcol[id], strtcol[id] + ncol, ysize);
    write_ppm_cols(argv[3], ncol, ysize, xsize, strtcol[id], cur_out, (char*) data);
    printf("P%d ends writing output\n", id);

    MPI_Barrier(MPI_COMM_WORLD);

    if (id == 0) {
        printf ("Took %g seconds\n", MPI_Wtime() - start);
    }

    MPI_Finalize();

    return(0);
}
