#ifndef V4L2CONTROL_H
#define V4L2CONTROL_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h> // low-level i/o 
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h> // for videodev2.h 
#include <linux/videodev2.h>
#include "test.h"
extern int GRAB_WIDTH, GRAB_HEIGHT; // размеры изображения с граббера

unsigned int grab_init(char *dev, int chnum);
void grab_set_chan(int ch_num);
unsigned char* grab_next();
void change_controls(int brightness, int contrast);
#endif //V4L2CONTROL_H

