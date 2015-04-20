#include "thresfilter.h"

#include <stdlib.h>
#include <pthread.h>

#define uint unsigned int

#define NUMTHREAD 8

typedef struct thread_param_t {
    int startline;
    int n;
} thread_param_t;

pixel * image;
int xsiz, ysiz;

int thresc = 0;

pthread_mutex_t thres_calc_lock;
pthread_mutex_t thres_div_lock;
pthread_cond_t calc_ready;
int calc_done = 0;
int div_done  = 0;

void * thresfilter_lines (void * void_params)
{
    int startline = ((thread_param_t *) void_params)->startline,
        n   = ((thread_param_t *) void_params)->n;

    int sum = 0;

    pixel * start = image + xsiz * startline,
          * end   = start + xsiz * n;

    pixel * ptr;

    for(ptr = start; ptr < end; ++ptr) {
        sum += (uint)ptr->r + (uint)ptr->g + (uint)ptr->b;
    }

    pthread_mutex_lock(&thres_calc_lock);
    thresc += sum;
    ++calc_done;

    while (calc_done != NUMTHREAD) {
        pthread_cond_wait(&calc_ready, &thres_calc_lock);
    }
    pthread_cond_signal(&calc_ready);

    pthread_mutex_unlock(&thres_calc_lock);

    pthread_mutex_lock(&thres_div_lock);

    if (!div_done) {
        thresc /= (xsiz * ysiz);
        div_done = 1;
    }

    pthread_mutex_unlock(&thres_div_lock);

    for (ptr = start; ptr < end; ++ptr) {
        sum = (uint)ptr->r + (uint)ptr->g + (uint)ptr->b;
        if(sum < thresc) {
            ptr->r = ptr->g = ptr->b = 0;
        }
        else {
            ptr->r = ptr->g = ptr->b = 255;
        }
    }

}

void thresfilter(const int xsize, const int ysize, pixel* src)
{
    thread_param_t * params;
    pthread_t      * threads;

    params  = (thread_param_t *) malloc (NUMTHREAD * sizeof (thread_param_t));
    threads = (pthread_t      *) malloc (NUMTHREAD * sizeof (pthread_t));

    // Initializing global shared variables

    xsiz = xsize;
    ysiz = ysize;
    image = src;

    // Launching all threads with right parameters

    int i, start = 0;
    for (i = 0; i < NUMTHREAD; ++i) {

        // Calculate number of line to handle per thread
        int nline = ysize / NUMTHREAD;
        if (ysize % NUMTHREAD > i)
            ++nline;

        // Store parameters in a struct to send them as one parameter
        params[i].startline = start;
        params[i].n         = nline;

        // Launch the thread
        pthread_create(threads + i, NULL, thresfilter_lines, params + i);

        // The next thread will handle lines after the last thread
        start += nline;
    }

    // Waiting all threads

    for (i = 0; i < NUMTHREAD; ++i) {
        pthread_join(threads[i], NULL);
    }
}
