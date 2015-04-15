/*
  File: blurfilter.c

  Implementation of blurfilter function.

 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi/mpi.h>
#include "blurfilter.h"
#include "ppmio.h"

#include <assert.h> // memory calculation checks

#define DBG

pixel* pix(pixel* image, const int xx, const int yy, const int xsize)
{
    register int off = xsize*yy + xx;

#ifdef DBG
    if(off >= MAX_PIXELS) {
        fprintf(stderr, "\n Terribly wrong: %d %d %d\n",xx,yy,xsize);
    }
#endif
    return image + off;
}

void blurfilter_x(const int xsize, const int ysize, pixel *src, const int radius, const double *w)
{
    int x,y,x2, wi;
    double r,g,b,n, wc;
    pixel * dst = new pixel[xsize * ysize];

    int id;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    for (y=0; y<ysize; y++) {
        for (x=0; x<xsize; x++) {

            r = w[0] * pix(src, x, y, xsize)->r;
            g = w[0] * pix(src, x, y, xsize)->g;
            b = w[0] * pix(src, x, y, xsize)->b;
            n = w[0];

            // for each radius offset
            for ( wi=1; wi <= radius; wi++) {
                wc = w[wi];
                // get x position compared to offset left
                x2 = x - wi;

                if(x2 >= 0) {
                    r += wc * pix(src, x2, y, xsize)->r;
                    g += wc * pix(src, x2, y, xsize)->g;
                    b += wc * pix(src, x2, y, xsize)->b;
                    n += wc;
                }

                // get x position compared to offset right
                x2 = x + wi;
                if(x2 < xsize) {
                    r += wc * pix(src, x2, y, xsize)->r;
                    g += wc * pix(src, x2, y, xsize)->g;
                    b += wc * pix(src, x2, y, xsize)->b;
                    n += wc;
                }
            }

            pix(dst,x,y, xsize)->r = r/n;
            pix(dst,x,y, xsize)->g = g/n;
            pix(dst,x,y, xsize)->b = b/n;
        }
    }
    memcpy(src, dst, xsize * ysize * 3);
}

void blurfilter_y(const int xsize, const int ysize, pixel *src, const int radius, const double *w)
{
    int x,y,y2, wi;
    double r,g,b,n, wc;
    pixel dst[ysize * xsize];

    // for each pixels
    for (y=0; y<ysize; y++) {
        for (x=0; x<xsize; x++) {
            //get their color components
            r = w[0] * pix(src, x, y, xsize)->r;
            g = w[0] * pix(src, x, y, xsize)->g;
            b = w[0] * pix(src, x, y, xsize)->b;
            n = w[0];
            for ( wi=1; wi <= radius; wi++) {
                wc = w[wi];
                y2 = y - wi;
                if(y2 >= 0) {
                    r += wc * pix(src, x, y2, xsize)->r;
                    g += wc * pix(src, x, y2, xsize)->g;
                    b += wc * pix(src, x, y2, xsize)->b;
                    n += wc;
                }
                y2 = y + wi;
                if(y2 < ysize) {
                    r += wc * pix(src, x, y2, xsize)->r;
                    g += wc * pix(src, x, y2, xsize)->g;
                    b += wc * pix(src, x, y2, xsize)->b;
                    n += wc;
                }
            }
            pix(dst,x,y, xsize)->r = r/n;
            pix(dst,x,y, xsize)->g = g/n;
            pix(dst,x,y, xsize)->b = b/n;
        }
    }
    memcpy(src, dst, xsize * ysize * 3);

}

