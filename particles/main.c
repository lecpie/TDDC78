#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "definitions.h"
#include "coordinate.h"
#include "physics.h"


#define MAXTIME 1000

float rand_range (float min, float max)
{
    return min + ((float) rand() / (float) RAND_MAX) * (max - min);
}

float rand_max (float max)
{
    return rand_range(0, max);
}

int main (void)
{
    particle_t ** particles;
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
    particles = malloc (MAX_NO_PARTICLES * sizeof(particle_t*));

    npart = INIT_NO_PARTICLES;
    momentum = 0.0;

    for (ipart = 0; ipart < npart; ++ipart) {
        particles[ipart] = malloc(sizeof(particle_t));

        // TODO Check what initial values should be
        particles[ipart]->ptype = 0;
        particles[ipart]->pcord.x = rand_max(BOX_HORIZ_SIZE);
        particles[ipart]->pcord.y = rand_max(BOX_VERT_SIZE);

        float r = rand_max ((float) MAX_INITIAL_VELOCITY),
              t = rand_max (2 * PI);

        particles[ipart]->pcord.vx = r * cos(t);
        particles[ipart]->pcord.vy = r * sin(t);
    }

    // For each time spec
    for (itime = 0, timestep = 0.0; itime < MAXTIME; ++itime, timestep += STEP_SIZE) {

        // For each particle
        for (ipart = 0; ipart < npart; ++ipart) {

            int collided = 0;

            // Check collisions
            for (jpart = ipart + 1; jpart < npart; ++jpart) {
                float t = collide(&particles[ipart]->pcord, &particles[jpart]->pcord);

                if (t != -1) {
                    interact(&particles[ipart]->pcord, &particles[jpart]->pcord, t);

                    collided = 1;

                    // Collided with j, checking wall interaction with j also before swapping it
                    momentum += wall_collide(&particles[jpart]->pcord, wall);

                    // particle j is put into i position, particle i is advanced to i+1
                    // and particle i+i is put into j position. i is incremented
                    // This way we avoid handling particle j again and potentialy move it.
                    particle_t * swap;
                    swap = particles[ipart];
                    particles[ipart] = particles[jpart];
                    particles[jpart] = particles[ipart + 1];
                    particles[++ipart] = swap;
                }
            }

            if (!collided) {
                // Move particles that has not collided with any another.
                feuler(&particles[ipart]->pcord, timestep);
            }

            // Check for wall interaction and add the momentum
            momentum += wall_collide(&particles[ipart]->pcord, wall);
        }

        //Comm if needed
    }

    // Calculate pressure
    pressure = momentum / (float) (itime + WALL_LENGTH);

    printf ("Pressure after %d timesteps : %g\n", itime, pressure);

    return 0;
}
