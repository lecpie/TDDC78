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
    return ( (float)id * (BOX_VERT_SIZE /  (float)np));
}

float getEProc (int id, int np)
{
    return getBProc(id + 1, np);
}

int getProcess(float y, int np)
{
	int processo = (int) (y / (BOX_VERT_SIZE / (float) np));
	//printf("processo : %d, %g\n",processo, y);
    return processo;
}

int main (int argc, char ** argv)
{

	int id, np, i;
    double start, end;
	unsigned ipart, jpart, npart, itime;
    float timestep, momentum, pressure;

	MPI_Init(&argc, &argv);
    MPI_Comm_size( MPI_COMM_WORLD, &np );
    MPI_Comm_rank( MPI_COMM_WORLD, &id );
	MPI_Status status;
	
	
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

    

    
    particle_t * particles, **particle_buffers, ** particle_buffers_recv;
   int buffer_sizes[np];
   
	
    particle_buffers = malloc (sizeof(particle_t *) * np );
    
	for(i=0; i<np; i++)
		particle_buffers[i] = malloc(sizeof(particle_t)*MAX_NO_PARTICLES);
	
    cord_t wall;

    
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


    npart = INIT_NO_PARTICLES; // /np
    momentum = 0.0;

    float miny = getBProc(id, np),
          maxy = getEProc(id, np);

	// start the timer
    start = MPI_Wtime();

	// assign particles to processors
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

		for( i= 0; i< np; i++)
			buffer_sizes[i] = 0;
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

        //COMMUNICATE THE MOVED PARTICLES
        for (ipart = 0; ipart < npart; ++ipart) {
            // Check for wall interaction and add the momentum
            momentum += wall_collide(&particles[ipart].pcord, wall);

            int process = getProcess(particles[ipart].pcord.y, np);
            if (process != id) {				//buffer_sizes[process] += 1;
				particle_buffers[process][buffer_sizes[process]++] = particles[ipart];
                // Swap with the last particle and remove the last
                particles[ipart] = particles[--npart];
            }
        }
        
        int p;
        for(p = 0;  p < np; p++){		            
			if(p != id){
				// send the particles moved in a buffer
				MPI_Send(&buffer_sizes[p], 1, MPI_INT, p, 0, MPI_COMM_WORLD);
				MPI_Send(particle_buffers[p], buffer_sizes[p], mpi_particle_t, p, 0, MPI_COMM_WORLD);
				
				int size = 0;
				//eventually receive new particles
				MPI_Recv(&size, 1, MPI_INT, p, 0, MPI_COMM_WORLD, &status);
				MPI_Recv(particles  + npart, size, mpi_particle_t, p, 0, MPI_COMM_WORLD, &status);
				npart += size;
			}
		}

/*		for(p = 0; p!= id && p < np; p++){
			MPI_Recv(particle_buffers_recv, 4*500, MPI_FLOAT, p, 0, MPI_COMM_WORLD, &status);
			//printf(" process %d , #particles %d\n", p,buffer_sizes[p]);
		}*/

    }
				
				
	printf("process #%d \t total number of particles: %d\n",id,npart);
	int nparticles = 0;
	MPI_Reduce(&npart,&nparticles,1,MPI_INT,MPI_SUM, MASTER, MPI_COMM_WORLD);

	
	
	float reduced_momentum  = 0.0;
	// Reduce the momentum computation on the master process
	MPI_Reduce(&momentum,&reduced_momentum,1,MPI_FLOAT,MPI_SUM, MASTER, MPI_COMM_WORLD);
	if(id == MASTER){
		// stop the timer
		end = MPI_Wtime()-start;
		printf("MASTER mpi time: %g sec \n\n\n",end);

		
		momentum = reduced_momentum;
        // Calculate pressure
        pressure = momentum / (float) (itime * WALL_LENGTH);

        printf("#### Pressure after %d timesteps : %g, number of particles: %d \n", itime, pressure,nparticles);
        printf("#### MASTER mpi time: %g sec \n\n\n",end);
    }

    MPI_Finalize();

    return 0;
}
