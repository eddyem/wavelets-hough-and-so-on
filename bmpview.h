#ifndef _BMPVIEW_H_
#define _BMPVIEW_H_
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include "test.h"
#include "events.h"
#include "v4l2control.h"

extern pthread_mutex_t m_ima, m_wvlt, m_hist;
void createWindow(int *window);
bool destroyWindow(int window);
void BMPview();
void renderBitmapString(float x, float y, void *font, char *string, GLubyte *color);
void Redraw();
void RedrawWindow();
void Resize(int width, int height);


#endif // _BMPVIEW_H_
