#include "events.h"
#include "wavedec.h"
/*
 *	Функции для работы с OpenGL'ными событиями
 */

int ShowWavelet = 0;

void keyPressed(unsigned char key,	// код введенного символа
				int x, int y){		// координаты мыши при нажатии клавиши
	int mod = glutGetModifiers(), window = glutGetWindow();
//	fprintf(stderr, "Key pressed. mod=%d, keycode=%d\n", mod, key);
	if((mod == GLUT_ACTIVE_CTRL) && (key == 'q' || key == 17)) exit(0); // ctrl + q
	switch(key){
		case 'i': more_info = !more_info;
			break;
		case 27: destroyWindow(window);
			break;
		case 'm': createWindow(&mainWindow);
			break;
		case 'w': createWindow(&WaveWindow);
			break;
		case 's': ShowWavelet = !ShowWavelet;
			break;
		case 'Z': Z += 1.0;
			break;
		case 'z': Z -= 1.0;
			break;
		case 'h': createWindow(&SubWindow);
			break;
	}
}

void keySpPressed(int key, int x, int y){
//	int mod = glutGetModifiers();
//	fprintf(stderr, "Sp. key pressed. mod=%d, keycode=%d\n", mod, key);
}

void mousePressed(int key, int state, int x, int y){
// GLUT_LEFT_BUTTON, GLUT_MIDDLE_BUTTON, GLUT_RIGHT_BUTTON 
//	int mod = glutGetModifiers();
//	fprintf(stderr, "Mouse button %s. point=(%d, %d); mod=%d, button=%d\n", 
//		(state == GLUT_DOWN)? "pressed":"released", x, y, mod, key);
	int window = glutGetWindow();
	if(window == WaveWindow){ // щелкнули в окне с вейвлетом
		int w = glutGet(GLUT_WINDOW_WIDTH) / 2;
		int h = glutGet(GLUT_WINDOW_HEIGHT) / 2;
		if(state == GLUT_DOWN && key == GLUT_LEFT_BUTTON){
			HistCoord[0] = (x > w);
			HistCoord[1] = (y > h);
		}
	}
}

void mouseMove(int x, int y){
//	fprintf(stderr, "Mouse moved to (%d, %d)\n", x, y);
}

void createMenu(int *window){
	glutCreateMenu(menuEvents);
	glutAddMenuEntry("Quit (ctrl+q)", 'q');
	glutAddMenuEntry("Close this window (ESC)", 27);
	if(window == &mainWindow){ // пункты меню главного окна
		glutAddMenuEntry("Info in stderr (i)", 'i');
	}
	else{
		glutAddMenuEntry("Open main window (m)", 'm');
	}
	if(window == &WaveWindow){
	}
	else{
		glutAddMenuEntry("Open wavelet window (w)", 'w');
	}
	if(window == &SubWindow){
	}
	else{
		glutAddMenuEntry("Open histogram window (h)", 'h');
	}
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void menuEvents(int opt){
	if(opt == 'q') exit(0);
	keyPressed((unsigned char)opt, 0, 0);
}
