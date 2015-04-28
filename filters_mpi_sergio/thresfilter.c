#include "thresfilter.h"
#include "mpi.h"
void thresfilter(const int xsize, const int ysize, pixel* src){
#define uint unsigned int 

  uint sum, i, psum, nump, rcvsum, rcvnump;

  nump = xsize * ysize;

  for(i = 0, sum = 0; i < nump; i++) {
    sum += (uint)src[i].r + (uint)src[i].g + (uint)src[i].b;
  }


// MPI_ALLREDUCE !
	MPI_Allreduce(&sum, &rcvsum, 1,	MPI_UNSIGNED,MPI_SUM, MPI_COMM_WORLD);
	MPI_Allreduce(&nump, &rcvnump, 1,MPI_UNSIGNED,MPI_SUM, MPI_COMM_WORLD);
	
	sum = rcvsum;
	
	nump = rcvnump;
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
