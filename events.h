#ifndef _EVENTS_H_
#define _EVENTS_H_
//#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <GL/glext.h>
#include "bmpview.h"

extern int *Histogram[3];			// ������ - ����������� (256 ������� ��� ������� �����)
extern int HistCoord[2];			// ���������� ���������� ��� ���������� �����������
extern float HMin, dI;				// ����� ������� �����������, ��� �� ��������������
extern GLubyte *BitmapBits;			// �������� � �������� (bmpview.c)
extern GLubyte *WaveletBits;		// �������������� ������� �������-����� (wavedec.c)
extern GLubyte *WVLT;				// �������� �������-��������������
extern int ShowWavelet;				// ����������� �������-������ � �������� ����
extern int wImageSize;				// ������ ����������� �������-������
extern int mainWindow, WaveWindow,	// �������������� ����
	SubWindow;
extern int more_info;				// ==1 - � stderr ��������� ��������� �� ����������� ����������
extern int totWindows;				// ����� ���-�� �������� ����
extern float Widths[2][2], Areas[2][2],// ������ � ������� ���������� �������-���������
			QMax[2][2], QMin[2][2]; // ������������ � ����������� ��������

extern float Z; // ���������� Z (zoom)

void keyPressed(unsigned char key, int x, int y);
void keySpPressed(int key, int x, int y);
void mousePressed(int key, int state, int x, int y);
void mouseMove(int x, int y);
void createMenu(int *window);
void menuEvents(int opt);
#endif // _EVENTS_H_
