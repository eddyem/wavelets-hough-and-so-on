#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <stdbool.h>
#include <jpeglib.h>
#include <jerror.h>
#include "test.h"
#include "wavedec.h"
#include "events.h"
#include "v4l2control.h"

extern void BMPview();
extern void Redraw();
pthread_t thread, thread_m;
GLubyte *BitmapBits = NULL;

struct option long_option[] =
{
	{"help", 0, NULL, 'h'},		// вывод справки
	{"device", 1, NULL, 'd'},	// устройство /dev/videoX
	{"channel", 1, NULL, 'c'},	// канал
	{NULL, 0, NULL, 0},
};

GLubyte *uncompressImage();
void help(char *s);
void *process_grabbing();

