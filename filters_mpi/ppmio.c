/*
  File: ppmio.c

  Implementation of PPM image file IO functions.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mpi/mpi.h>

#include "ppmio.h"

int read_ppm (const char * fname, 
	       int * xpix, int * ypix, int * max, char * data) {
  char ftype[40];
  char ctype[40] = "P6";
  char line[80];
  int errno;

  FILE * fp;
  errno = 0;

  if (fname == NULL) fname = "\0";
  fp = fopen (fname, "r");
  if (fp == NULL) {
    fprintf (stderr, "read_ppm failed to open %s: %s\n", fname,
	     strerror (errno));
    return 1;
  }
  
  fgets(line, 80, fp);
  sscanf(line, "%s", ftype);

  while (fgets(line, 80, fp) && (line[0] == '#'));

  sscanf(line, "%d%d", xpix, ypix);
  fscanf(fp, "%d\n", max);

  if(*xpix * *ypix > MAX_PIXELS) {
     fprintf (stderr, "Image size is too big\n");
    return 4;
 };

  if (strncmp(ftype, ctype, 2) == 0) {
    if (fread (data, sizeof (char), *xpix * *ypix * 3, fp) != 
	*xpix * *ypix * 3) {
      perror ("Read failed");
      return 2;
    }
  } else {
    fprintf (stderr, "Wrong file format: %s\n", ftype);
  }

  if (fclose (fp) == EOF) {
    perror ("Close failed");
    return 3;
  }
  return 0;

}

int read_ppm_head (const char * fname, int * xpix, int * ypix, int * max, int * cur)
{
    char ftype[40];
    char ctype[40] = "P6";
    char line[80];
    int errno;

    FILE * fp;
    errno = 0;

    if (fname == NULL) fname = "\0";
    fp = fopen (fname, "r");
    if (fp == NULL) {
      fprintf (stderr, "read_ppm failed to open %s: %s\n", fname,
           strerror (errno));
      return 1;
    }

    fgets(line, 80, fp);
    sscanf(line, "%s", ftype);

    while (fgets(line, 80, fp) && (line[0] == '#'));

    sscanf(line, "%d%d", xpix, ypix);
    fscanf(fp, "%d\n", max);

    if(*xpix * *ypix > MAX_PIXELS) {
       fprintf (stderr, "Image size is too big\n");
      return 4;
   };

    if (strncmp(ftype, ctype, 2) != 0) {
        fprintf (stderr, "Wrong file format: %s\n", ftype);
        return 5;
    }

    *cur = ftell(fp);

    if (fclose (fp) == EOF) {
      perror ("Close failed");
      return 3;
    }

    return 0;
}

int read_ppm_data (const char * fname, int xpix, int ypix, int cur, char *data, int * end)
{
    int errno = 0;
    FILE * fp;

    if (fname == NULL) fname = "\0";
    fp = fopen (fname, "r");
    if (fp == NULL) {
      fprintf (stderr, "read_ppm failed to open %s: %s\n", fname, strerror (errno));
      return 1;
    }

    fseek(fp, cur, SEEK_SET);

    int i;

    int id;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    if (fread(data, sizeof(char), ypix * xpix * 3, fp) != ypix * xpix * 3)
        return 2;

    if (end != NULL)
        *end = ftell(fp);

    if (fclose (fp) == EOF) {
      perror ("Close failed");
      return 3;
    }
}

int write_ppm_head (const char * fname, int xpix, int ypix, int * off)
{
    FILE * fp;
    int errno = 0;

    if (fname == NULL) fname = "\0";
    fp = fopen (fname, "w");
    if (fp == NULL) {
      fprintf (stderr, "write_ppm failed to open %s: %s\n", fname,
           strerror (errno));
      return 1;
    }

    fprintf (fp, "P6\n");
    fprintf (fp, "%d %d 255\n", xpix, ypix);

    *off = ftell(fp);

    if (fclose (fp) == EOF) {
      perror ("Close failed");
      return 3;
    }
    return 0;
}

int write_ppm_lin (const char * fname, int xpix, int ypix, int cur, char * data)
{
    FILE * fp;
    int errno = 0;

    if (fname == NULL) fname = "\0";
    fp = fopen (fname, "r+");
    if (fp == NULL) {
      fprintf (stderr, "write_ppm failed to open %s: %s\n", fname,
           strerror (errno));
      return 1;
    }

    int id;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    fseek(fp, cur, SEEK_SET);

    fwrite (data, 3 * sizeof(char), xpix * ypix, fp);

    if (fclose (fp) == EOF) {
      perror ("Close failed");
      return 3;
    }
    return 0;
}

int write_ppm_cols (const char * fname, int xpix, int ypix, int xsize, int col_off, int cur, char * data) {
    FILE * fp;
    int errno = 0;

    if (fname == NULL) fname = "\0";
    fp = fopen (fname, "r+");
    if (fp == NULL) {
      fprintf (stderr, "write_ppm failed to open %s: %s\n", fname,
           strerror (errno));
      return 1;
    }

    int i, off = 0,
           file_off = cur + col_off * 3;
    fseek(fp, file_off, SEEK_SET);

    for (i = 0; i < ypix; ++i) {
        fwrite(data + off, sizeof(char) * 3, xpix, fp);
        off += xpix * 3;
        file_off += xsize * 3;
        fseek(fp, file_off, SEEK_SET);
    }

    if (fclose (fp) == EOF) {
      perror ("Close failed");
      return 3;
    }
    return 0;
}

int write_ppm (const char * fname, int xpix, int ypix, char * data) {

  FILE * fp;
  int errno = 0;

  if (fname == NULL) fname = "\0";
  fp = fopen (fname, "w");
  if (fp == NULL) {
    fprintf (stderr, "write_ppm failed to open %s: %s\n", fname,
	     strerror (errno));
    return 1;
  }
  
  fprintf (fp, "P6\n");
  fprintf (fp, "%d %d 255\n", xpix, ypix);
  if (fwrite (data, sizeof (char), xpix*ypix*3, fp) != xpix*ypix*3) {
    perror ("Write failed");
    return 2;
  }
  if (fclose (fp) == EOF) {
    perror ("Close failed");
    return 3;
  }
  return 0;
}
