#include <stdio.h>
#include "mpi.h"

main(int argc, char** argv) {

    /* .......Variables Initialisation ......*/
    int Numprocs, MyRank, Root = 0;
    int index, RecvBuffer[80000];
    int *InputBuffer;
    int Scatter_DataSize;
    int DataSize;
    MPI_Status status;

    /* ........MPI Initialisation .......*/
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &MyRank);
    MPI_Comm_size(MPI_COMM_WORLD, &Numprocs);

    if (MyRank == Root) {
        DataSize = 80000;

        /* ...Allocate memory.....*/

        InputBuffer = (int*) malloc(DataSize * sizeof(int));

        for (index = 0; index < DataSize; index++)
            InputBuffer[index] = index;
    }

    MPI_Bcast(&DataSize, 1, MPI_INT, Root, MPI_COMM_WORLD);
    if (DataSize % Numprocs != 0) {
        if (MyRank == Root)
            printf("Input is not evenly divisible by Number of Processes\n");
        MPI_Finalize();
        exit(-1);
    }

    Scatter_DataSize = DataSize / Numprocs;
    //RecvBuffer = (int *) malloc(Scatter_DataSize * sizeof(int));

    MPI_Scatter(InputBuffer, Scatter_DataSize, MPI_INT, RecvBuffer,
            Scatter_DataSize, MPI_INT, Root, MPI_COMM_WORLD);

    for (index = 0; index < Scatter_DataSize; ++index)
        printf("MyRank = %d, RecvBuffer[%d] = %d \n", MyRank, index,
                RecvBuffer[index]);

    MPI_Finalize();

}
