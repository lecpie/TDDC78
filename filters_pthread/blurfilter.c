/*
  File: blurfilter.c

  Implementation of blurfilter function.

 */
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include "blurfilter.h"
#include "ppmio.h"


#define NUMTHREAD 16

pixel* pix(pixel* image, const int xx, const int yy, const int xsize)
{
    register int off = xsize*yy + xx;

#ifdef DBG
    if(off >= MAX_PIXELS) {
        fprintf(stderr, "\n Terribly wrong: %d %d %d\n",xx,yy,xsize);
    }
#endif
    return (image + off);
}

typedef struct param_t {
    int start_lin;
    int start_col;
    int n_lin;
    int n_col;
} param_t;

// Global shared variables

pixel * image,
      * image_swap;

const double * wgt;

int xsiz, ysiz, rad;

int lin_done = 0;
pthread_mutex_t lock_lins;
pthread_cond_t  cond_lins;

void * blurfilter_thr  (void * arg)
{
    param_t * param = (param_t *) arg;

    int start_lin = param->start_lin,
        start_col = param->start_col,
        n_lin     = param->n_lin,
        n_col     = param->n_col;

    int end_y = start_lin + n_lin,
        end_x = start_col + n_col;

    int y, x, wi, x2, y2;

    double r, g, b, n, wc;

    for (y = start_lin; y < end_y; ++y) {
        for (x = 0; x < xsiz; ++x) {
            r = wgt[0] * pix(image, x, y, xsiz)->r;
            g = wgt[0] * pix(image, x, y, xsiz)->g;
            b = wgt[0] * pix(image, x, y, xsiz)->b;
            n = wgt[0];
            for ( wi = 1; wi <= rad; ++wi) {
                wc = wgt[wi];
                x2 = x - wi;
                if(x2 >= 0) {
                    r += wc * pix(image, x2, y, xsiz)->r;
                    g += wc * pix(image, x2, y, xsiz)->g;
                    b += wc * pix(image, x2, y, xsiz)->b;
                    n += wc;
                }
                x2 = x + wi;
                if(x2 < xsiz) {
                    r += wc * pix(image, x2, y, xsiz)->r;
                    g += wc * pix(image, x2, y, xsiz)->g;
                    b += wc * pix(image, x2, y, xsiz)->b;
                    n += wc;
                }
            }
            pix(image_swap,x,y, xsiz)->r = r/n;
            pix(image_swap,x,y, xsiz)->g = g/n;
            pix(image_swap,x,y, xsiz)->b = b/n;

        }
    }

    pthread_mutex_lock(&lock_lins);

    ++lin_done;
    while (lin_done != NUMTHREAD) {
        pthread_cond_wait(&cond_lins, &lock_lins);
    }
    pthread_cond_signal(&cond_lins);

    pthread_mutex_unlock(&lock_lins);

    for (y = 0; y < ysiz; ++y) {
        for (x = start_col; x < end_x; ++x) {
            r = wgt[0] * pix(image_swap, x, y, xsiz)->r;
            g = wgt[0] * pix(image_swap, x, y, xsiz)->g;
            b = wgt[0] * pix(image_swap, x, y, xsiz)->b;
            n = wgt[0];
            for (wi = 1; wi <= rad; ++wi) {
                wc = wgt[wi];
                y2 = y - wi;
                if(y2 >= 0) {
                    r += wc * pix(image_swap, x, y2, xsiz)->r;
                    g += wc * pix(image_swap, x, y2, xsiz)->g;
                    b += wc * pix(image_swap, x, y2, xsiz)->b;
                    n += wc;
                }
                y2 = y + wi;
                if(y2 < ysiz) {
                    r += wc * pix(image_swap, x, y2, xsiz)->r;
                    g += wc * pix(image_swap, x, y2, xsiz)->g;
                    b += wc * pix(image_swap, x, y2, xsiz)->b;
                    n += wc;
                }
            }
            pix(image,x,y, xsiz)->r = r/n;
            pix(image,x,y, xsiz)->g = g/n;
            pix(image,x,y, xsiz)->b = b/n;
        }
    }

}

void blurfilter(const int xsize, const int ysize, pixel* src, const int radius, const double *w)
{
    int size = xsize * ysize;

    // Copy of some parameters into global shared variable
    xsiz = xsize;
    ysiz = ysize;
    image = src;
    rad = radius;
    wgt = w;

    image_swap = (pixel *) malloc(sizeof(pixel) * size);

    pthread_t threads[NUMTHREAD];
    param_t   args   [NUMTHREAD];

    int i;

    int n_lin = ysize / NUMTHREAD,
        n_col = xsize / NUMTHREAD,
        lin_lft = ysize % NUMTHREAD,
        col_lft = xsize % NUMTHREAD,
        start_lin = 0,
        start_col = 0;


    for (i = 0; i < NUMTHREAD; ++i) {
        args[i].n_col = (col_lft > i) ? n_col + 1 : n_col;
        args[i].n_lin = (lin_lft > i) ? n_lin + 1 : n_lin;
        args[i].start_col = start_col;
        args[i].start_lin = start_lin;

        start_col += args[i].n_col;
        start_lin += args[i].n_lin;

        pthread_create(threads + i, NULL, blurfilter_thr, args + i);
    }

    for (i = 0; i < NUMTHREAD; ++i)
        pthread_join(threads[i], NULL);

    free(image_swap);
}



