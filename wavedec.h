#ifndef _WAVEDEC_H_
#define _WAVEDEC_H_
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include <stdbool.h>

extern double *SinBuf;
void fillSinBuf();
void mkWavedec();
void buildHistogram();
void initWavelet(bool add);

#endif // _WAVEDEC_H_
