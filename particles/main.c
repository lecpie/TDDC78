#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <mpi.h>

#include "definitions.h"
#include "coordinate.h"
#include "physics.h"

#define MAXTIME 1000

#define MASTER 0

float rand_range (float min, float max)
{
    return min + ((float) rand() / (float) RAND_MAX) * (max - min);
}

float rand_max (float max)
{
    return rand_range(0, max);
}

float getBProc (int id, int np)
{
    return id * (BOX_VERT_SIZE / (float) np);
}

float getEProc (int id, int np)
{
    return getBProc(id + 1, np);
}

int getProcess(float y, int np)
{
    return (int) (BOX_VERT_SIZE / np);
}

int main (int argc, char ** argv)
{
    MPI_Init(&argc, &argv);

    /* MPI_DATATYPE : mpi_particle_t */
    MPI_Datatype mpi_particle_t;
    const int nitems=4;
    int          blocklengths[4] = {sizeof(float), sizeof(float), sizeof(float), sizeof(float)};
    MPI_Datatype types[4] = {MPI_FLOAT, MPI_FLOAT, MPI_FLOAT, MPI_FLOAT};
    MPI_Aint     offsets[4];

    offsets[0] = 0;
    offsets[1] = sizeof(float);
    offsets[2] = 2 * sizeof(float);
    offsets[3] = 3 * sizeof(float);
    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_particle_t);
    MPI_Type_commit(&mpi_particle_t);

    int id, np, i;

    MPI_Comm_size( MPI_COMM_WORLD, &np );
    MPI_Comm_rank( MPI_COMM_WORLD, &id );

    particle_t * particles, **particle_buffers;
    int * buffer_sizes;

    particle_buffers = malloc (sizeof(particle_t *) * np);
    buffer_sizes      = malloc (sizeof(int) * np);



    cord_t wall;

    unsigned ipart, jpart, npart, itime;
    float timestep, momentum, pressure;

    // Init random generator
    srand((unsigned)time(NULL));

    // Init wall
    wall.x0 = 0.0;
    wall.y0 = 0.0;
    wall.x1 = BOX_HORIZ_SIZE;
    wall.y1 = BOX_VERT_SIZE;

    // Initialize particles
    // Each processor should have its own particle array
    particles = malloc (MAX_NO_PARTICLES * sizeof(particle_t));


    npart = INIT_NO_PARTICLES;
    momentum = 0.0;

    float miny = getBProc(id, np),
          maxy = getEProc(id, np);

    for (ipart = 0; ipart < npart; ++ipart) {

        // TODO Check what initial values should be
        particles[ipart].ptype = 0;
        particles[ipart].pcord.x = rand_max(BOX_HORIZ_SIZE);
        particles[ipart].pcord.y = rand_range(miny, maxy);

        float r = rand_max ((float) MAX_INITIAL_VELOCITY),
              t = rand_max (2 * PI);

        particles[ipart].pcord.vx = r * cos(t);
        particles[ipart].pcord.vy = r * sin(t);
    }

    // For each time spec
    for (itime = 0, timestep = 0.0; itime < MAXTIME; ++itime, timestep += STEP_SIZE) {

        // For each particle
        for (ipart = 0; ipart < npart; ++ipart) {

            int collided = 0;

            // Check collisions
            for (jpart = ipart + 1; jpart < npart; ++jpart) {
                float t = collide(&particles[ipart].pcord, &particles[jpart].pcord);

                if (t != -1) {
                    interact(&particles[ipart].pcord, &particles[jpart].pcord, t);

                    collided = 1;

                    // particle j is put into i position, particle i is advanced to i+1
                    // and particle i+i is put into j position. i is incremented
                    // This way we avoid handling particle j again and potentialy move it.
                    particle_t swap;
                    swap = particles[ipart];
                    particles[ipart] = particles[jpart];
                    particles[jpart] = particles[ipart + 1];
                    particles[++ipart] = swap;
                }
            }

            if (!collided) {
                // Move particles that has not collided with any another.
                feuler(&particles[ipart].pcord, STEP_SIZE);
            }

        }

        //Comm if needed

        for (ipart = 0; ipart < npart; ++ipart) {
            // Check for wall interaction and add the momentum
            momentum += wall_collide(&particles[ipart].pcord, wall);

            int process;
            if ((process = getProcess(particles[ipart].pcord.y, np)) != id) {
                MPI_Send(particles + ipart, 4, MPI_FLOAT, process, 0, MPI_COMM_WORLD);

                // Swap with the last particle and remove the last
                particles[ipart] = particles[--npart];
            }
        }



    }

    if (id != MASTER) {
        MPI_Send(&momentum, 1, MPI_FLOAT, MASTER, 0, MPI_COMM_WORLD);
    }
    else {
        float sum;
        MPI_Status status;
        int expected = np - 1;

        while (expected--) {
            MPI_Recv(&sum, 1, MPI_FLOAT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            momentum += sum;
        }

        // Calculate pressure
        pressure = momentum / (float) (itime * WALL_LENGTH);

        printf ("Pressure after %d timesteps : %g\n", itime, pressure);
    }

    MPI_Finalize();

    return 0;
}
