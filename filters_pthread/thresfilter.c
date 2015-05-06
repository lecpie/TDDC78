#include "thresfilter.h"

#include <stdlib.h>
#include <pthread.h>

#define uint unsigned int


typedef struct param_t {
    int strt_line;
    int n_line;
} param_t;

pixel * image;
int xsiz, ysiz;

long long unsigned int thresc = 0;

pthread_mutex_t thres_calc_lock;
pthread_mutex_t thres_div_lock;
pthread_cond_t calc_ready;
int calc_done = 0;
int div_done  = 0;
unsigned numthread;

void * thresfilter_lines (void * void_params)
{
    int startline = ((param_t *) void_params)->strt_line,
        n         = ((param_t *) void_params)->n_line;

    long long unsigned int sum = 0;

    pixel * start = image + xsiz * startline,
          * end   = start + xsiz * n;

    pixel * ptr;

    for(ptr = start; ptr < end; ++ptr) {
        sum += (uint)ptr->r + (uint)ptr->g + (uint)ptr->b;
    }

    pthread_mutex_lock(&thres_calc_lock);
    thresc += sum;
    ++calc_done;

    while (calc_done != numthread) {
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

void thresfilter(const int xsize, const int ysize, pixel* src, unsigned nthread)
{
    param_t   * params  = malloc (sizeof(param_t)   * nthread);
    pthread_t * threads = malloc (sizeof(pthread_t) * nthread);

    // Initializing global shared variables

    xsiz = xsize;
    ysiz = ysize;
    image = src;
    numthread = nthread;

    // Calculate number of line to handle per thread

    int n_lin   = ysize / nthread,
        lft_lin = ysize % nthread;

    // Launching all threads with right parameters

    int i, start;

    for (i = 0, start = 0; i < nthread; ++i) {

        // Store parameters in a struct to send them as one parameter
        params[i].strt_line = start;
        params[i].n_line    = (lft_lin > i) ? n_lin + 1 : n_lin;

        // Launch the thread
        pthread_create(threads + i, NULL, thresfilter_lines, params + i);

        // The next thread will handle lines after the last thread
        start += params[i].n_line;
    }

    // Waiting all threads

    for (i = 0; i < nthread; ++i) {
        pthread_join(threads[i], NULL);
    }
}
