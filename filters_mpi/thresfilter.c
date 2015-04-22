#include "thresfilter.h"

#define uint unsigned int

void thresfilter(const int xsize, const int ysize, pixel* src){
  uint sum, i, psum, nump;

  nump = xsize * ysize;

  for(i = 0, sum = 0; i < nump; i++) {
    sum += (uint)src[i].r + (uint)src[i].g + (uint)src[i].b;
  }

  sum /= nump;

  for(i = 0; i < nump; i++) {
    psum = (uint)src[i].r + (uint)src[i].g + (uint)src[i].b;
    if(sum > psum) {
      src[i].r = src[i].g = src[i].b = 0;
    }
    else {
      src[i].r = src[i].g = src[i].b = 255;
    }
  }
}

void calc_sum (pixel * src, int size, int * sum)
{
    uint i;

    pixel * end = src + size;

    for(*sum = 0; src != end; ++src) {
      *sum += (uint)src[i].r + (uint)src[i].g + (uint)src[i].b;
    }
}

void calc_thresfilter (pixel * src, const int nump, const int sum)
{
    int i, psum;

    for(i = 0; i < nump; i++) {
      psum = (uint)src[i].r + (uint)src[i].g + (uint)src[i].b;
      if(sum > psum) {
        src[i].r = src[i].g = src[i].b = 0;
      }
      else {
        src[i].r = src[i].g = src[i].b = 255;
      }
    }
}
