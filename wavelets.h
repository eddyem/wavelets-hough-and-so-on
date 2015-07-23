// Onur G. Guleryuz 1995, 1996, 1997,
// University of Illinois at Urbana-Champaign,
// Princeton University,
// Polytechnic University.

#ifndef _WAVELETS_H_
#define _WAVELETS_H_

#define MAX_ARR_SIZE 64

typedef float** WImage;

WImage alloc_ima(int N,int M,char zero);

void free_ima(WImage a,int N);

float *allocate_1d_float(int N,char zero);

void wav2d_inpl(WImage image,int Ni,int Nj,int levs,char forw,int *shift_arr_row,int *shift_arr_col);

#endif
