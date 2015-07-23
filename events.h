#ifndef _EVENTS_H_
#define _EVENTS_H_
//#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <GL/glext.h>
#include "bmpview.h"

extern int *Histogram[3];			// массив - гистограмма (256 уровней для каждого цвета)
extern int HistCoord[2];			// координаты разложения для построения гистограммы
extern float HMin, dI;				// левая граница гистограммы, шаг по интенисивности
extern GLubyte *BitmapBits;			// картинка с граббера (bmpview.c)
extern GLubyte *WaveletBits;		// результирующий цветной вейвлет-образ (wavedec.c)
extern GLubyte *WVLT;				// обратное вейвлет-преобразование
extern int ShowWavelet;				// отображение вейвлет-образа в основном окне
extern int wImageSize;				// размер изображения вейвлет-образа
extern int mainWindow, WaveWindow,	// идентификаторы окон
	SubWindow;
extern int more_info;				// ==1 - в stderr выводятся сообщения об экстремумах гистограмм
extern int totWindows;				// общее кол-во открытых окон
extern float Widths[2][2], Areas[2][2],// ширины и площади гистограмм вейвлет-компонент
			QMax[2][2], QMin[2][2]; // максимальное и минимальное значения

extern float Z; // координата Z (zoom)

void keyPressed(unsigned char key, int x, int y);
void keySpPressed(int key, int x, int y);
void mousePressed(int key, int state, int x, int y);
void mouseMove(int x, int y);
void createMenu(int *window);
void menuEvents(int opt);
#endif // _EVENTS_H_
