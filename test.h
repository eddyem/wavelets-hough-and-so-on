#ifndef _TEST_H_
#define _TEST_H_
#include <stdbool.h>

//#define __TEST__  // отладочные сообщения в stderr
double dtime();
#ifdef __TEST__
#define WARN(...)  do{fprintf(stderr,__VA_ARGS__);}while(0)
#else
#define WARN(...)  do{}while(0)
#endif

#define ERR(...)  do{fprintf(stderr,__VA_ARGS__); fprintf(stderr, "%s\n", strerror(errno));}while(0)

#endif // _TEST_H_
