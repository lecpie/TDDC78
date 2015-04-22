#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <mpi.h>

#include "ppmio.h"
#include "thresfilter.h"

#define ROOTPROC 0

void die (int ret) {
    MPI_Finalize();
    exit(ret);
}

typedef struct arg_t {
    int start;
    int size;

} arg_t;

int main (int argc, char ** argv) {
    int xsize, ysize, colmax, n_lin;

    int i;

    int id, np;

    MPI_Request req;

    pixel * image;
    int size, totalsize;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    // Creating arg type

    arg_t args[np];

    if (id == ROOTPROC) {
        // Take care of the arguments

        if (argc != 3) {
            fprintf(stderr, "Usage: %s infile outfile\n", argv[0]);
            die(1);
        }

        image = malloc(3 * MAX_PIXELS);

        if (read_ppm(argv[1], &xsize, &ysize, &colmax, (char *) image))
            die(1);


        if (colmax > 255) {
            fprintf(stderr, "Too large maximum color-component value\n");
            die(1);
        }

        printf("P%d: Read file, sending tasks\n", id);

        totalsize = xsize * ysize;

        int start = 0;

        for (i = 0; i < np; ++i) {
            n_lin = ysize / np;
            if (ysize % np > id)
                ++n_lin;

            args[i].start = start;
            args[i].size  = n_lin * xsize;

            MPI_Isend(&args[i].size, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &req);
            MPI_Isend(&totalsize,    1, MPI_INT, i, 1, MPI_COMM_WORLD, &req);

            MPI_Isend(image + start, args[i].size * 3, MPI_CHAR, i, 2, MPI_COMM_WORLD, &req);

            start += args[i].size;
        }

    }

    MPI_Status status;

    MPI_Recv(&size,      1, MPI_INT, ROOTPROC, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(&totalsize, 1, MPI_INT, ROOTPROC, 1, MPI_COMM_WORLD, &status);

    pixel * src = malloc (3 * size);

    MPI_Recv(src, size * 3, MPI_CHAR, ROOTPROC, 2, MPI_COMM_WORLD, &status);

    int sum, sumcpy;

    calc_sum(src, size, &sum);

    sumcpy = sum;

    for (i = 0; i < np; ++i) {
        if (i == id) continue;

        MPI_Send(&sumcpy, 1, MPI_INT, i, 3, MPI_COMM_WORLD);
    }

    int expected = np - 1;
    int add_sum;

    while (expected--) {
        MPI_Recv(&add_sum, 1, MPI_INT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD, &status);

        sum += add_sum;
    }

    sum /= totalsize;

    printf("P%d: Calling filter...\n", id);

    calc_thresfilter(src, size, sum);

    printf("P%d: Filtering done, sending to root\n", id);


    MPI_Send(&size, 1, MPI_INT, ROOTPROC, 4, MPI_COMM_WORLD);
    MPI_Isend(src, size * 3, MPI_CHAR, ROOTPROC, 5, MPI_COMM_WORLD, &req);
    
    if (id == ROOTPROC) {
        expected = np;
        int received;

        printf ("P%d: Waiting result parts\n", id);

        MPI_Request assembl[np];

        while (expected--) {
            MPI_Recv(&received, 1, MPI_INT, MPI_ANY_SOURCE, 4, MPI_COMM_WORLD, &status);
            printf ("P%d: Receiving part from P%d, assembling...\n", id, status.MPI_SOURCE);

            MPI_Irecv(image + args[status.MPI_SOURCE].start, 3 * args[status.MPI_SOURCE].size,
                     MPI_CHAR, status.MPI_SOURCE, 5, MPI_COMM_WORLD, assembl + expected);
        }

        expected = np;
        while (expected--) {
            MPI_Wait(assembl + expected, &status);
        }

        printf("P%d: Result assembled, writing to file\n", id);

        if (write_ppm(argv[2], xsize, ysize, (char *) image) != 0) {
            perror("write_ppm");
        }
    }

    MPI_Finalize();

    return(0);
}
