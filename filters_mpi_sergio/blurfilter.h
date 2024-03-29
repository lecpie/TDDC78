/*
  File: blurfilter.h

  Declaration of pixel structure and blurfilter function.
    
 */

#ifndef _BLURFILTER_H_
#define _BLURFILTER_H_

/* NOTE: This structure must not be padded! */
typedef struct _pixel {
    unsigned char r,g,b;
} pixel;

void blurfilter(const int xsize, const int ysize, pixel* src, const int radius, const double *w);
void blurfilter_bordered(const int xsize, const int ysize, const int ycompute_start, const int ycompute_end, pixel* src, const int radius, const double *w);

#endif
