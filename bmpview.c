//      bmpview.c
//      
//      Copyright 2010 Edward V. Emelianoff <eddy@sao.ru>
//      
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//      
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//      
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.
//-lglut
#include "bmpview.h"

float Z = -100.;
/*
 * Functions...
 */
int mainWindow = -1, WaveWindow = -1, SubWindow = -1;
int totWindows = 0;
GLuint Tex[3];

void createWindow(int *window){
	char *Name;
	int n;
	if(*window != -1) return;
	if(window == &mainWindow){
		Name = strdup("Main window");
		n = 0;
	}
	else{
		if(window == &WaveWindow){
			Name = strdup("Wavelet viewer");
			n = 1;
		}
		else{
			Name = strdup("Subwindow");
			n = 2;
		}
	}
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(GRAB_WIDTH, GRAB_HEIGHT);
	*window = glutCreateWindow(Name);
	glutIdleFunc(Redraw);
	glutReshapeFunc(Resize);
	glutDisplayFunc(RedrawWindow);
	glutKeyboardFunc(keyPressed);
	glutSpecialFunc(keySpPressed);
	glutMouseFunc(mousePressed);
	glutMotionFunc(mouseMove);
	createMenu(window);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, Tex[n]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, GRAB_WIDTH, GRAB_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL); 
	totWindows++;
	free(Name);
}

bool destroyWindow(int window){
	if(window == -1) return 0;
	if(totWindows == 1) return 0;
//	if(mainWindow > -1){
//		glutSetWindow(mainWindow);
		glutDestroyWindow(window);
		if(window == mainWindow) mainWindow = -1;
		else if(window == WaveWindow) WaveWindow = -1;
		else if(window == SubWindow){
			int i;
			SubWindow = -1;
			for(i=0; i<3; i++){
				free(Histogram[i]);
				Histogram[i] = NULL;
			}
		}
		totWindows--;
		return 1;
//	}
//	return 0;
}

void BMPview(){
	char *argv[] = {"wavinfo", NULL};
	int argc = 1;
	glutInit(&argc, argv);
	glGenTextures(3, Tex);
	createWindow(&mainWindow);
	createWindow(&WaveWindow);
	glutMainLoop();
}

void renderBitmapString(float x, float y, void *font, char *string, GLubyte *color){
	char *c;
	int x1=x, W=0;
	for(c = string; *c; c++){
		W += glutBitmapWidth(font,*c);// + 1;
	}
	x1 -= W/2;
	glColor3ubv(color);
	glLoadIdentity();
	glTranslatef(0.,0., -150);
	//glTranslatef(x,y, -4000.);
	for (c = string; *c != '\0'; c++){
		glColor3ubv(color);
		glRasterPos2f(x1,y);
		glutBitmapCharacter(font, *c);
		//glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
		x1 = x1 + glutBitmapWidth(font,*c);// + 1;
	}
}

void Redraw(){
	if(mainWindow != -1){
		glutSetWindow(mainWindow);
		glutPostRedisplay();
	}
	if(WaveWindow != -1){
		glutSetWindow(WaveWindow);
		glutPostRedisplay();
	}
	if(SubWindow != -1){
		glutSetWindow(SubWindow);
		glutPostRedisplay();
	}
}

void RedrawWindow(){
	int window, n;
	pthread_mutex_t *mutex;
	GLubyte *ptr = NULL;
	window = glutGetWindow();
	/* Clear the window to black */
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(window == WaveWindow && WaveletBits){
		ptr = WaveletBits;
		mutex = &m_wvlt;
	}
	else{
		if(ShowWavelet && WaveletBits){
			ptr = WaveletBits;
			mutex = &m_wvlt;
		}
		else{
			//ptr = BitmapBits;
			ptr = WVLT;
			mutex = &m_ima;
		}
	}
	if(window == SubWindow && Histogram[0]){
		ptr = NULL;
		mutex = &m_hist;
	}
	if(pthread_mutex_trylock(mutex) != 0) return;
	if(ptr){
		if(ptr == WaveletBits) n = 1;
		else n = 0;
		glLoadIdentity();
		glTranslatef(-GRAB_WIDTH/2.,-GRAB_HEIGHT/2., Z);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, Tex[n]);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRAB_WIDTH, GRAB_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, ptr);
		glBegin(GL_QUADS); 
		glTexCoord2f(0., 1.); glVertex3f(0., 0., 0.); 
		glTexCoord2f(0., 0.); glVertex3f(0., GRAB_HEIGHT, 0.); 
		glTexCoord2f(1., 0.); glVertex3f(GRAB_WIDTH, GRAB_HEIGHT, 0.); 
		glTexCoord2f(1., 1.); glVertex3f(GRAB_WIDTH, 0., 0.); 
		glEnd();
		glDisable(GL_TEXTURE_2D);
		if(window == WaveWindow && fabs(Widths[0][0]) > 0.1){
			GLubyte red[] = {200, 10, 15};
			char *str = calloc(128, 1);
			int i, j;
			for(j=0; j<2; j++) for(i=0; i<2; i++){
				snprintf(str, 127, "(%d,%d)%.3g / %.3e", i,j, Widths[j][i], Widths[j][i]/Widths[0][0]);
				renderBitmapString(GRAB_WIDTH/4.*(-1.+2.*i), 40.*(1.-j)-15, GLUT_BITMAP_HELVETICA_18 , str, red);
				snprintf(str, 127, "%.3g / %.3e", Areas[j][i], Areas[j][i]/Areas[0][0]);
				renderBitmapString(GRAB_WIDTH/4.*(-1.+2.*i), 40.*(1.-j)-35, GLUT_BITMAP_HELVETICA_18 , str, red);
			}
			printf("%.3g\t%.3g\n", (Widths[0][1]+Widths[1][0])/Widths[0][0], Widths[1][1]/Widths[0][0]*14.);
			free(str);
		
			
		}
	}
	else if(Histogram[0]){
		int i, j, imax=0;
		float max_ = 0., max1, Imax;
		char *str = calloc(128,1);
		GLubyte white[] = {255, 255, 255};
		glLoadIdentity();
		glTranslatef(32.-GRAB_WIDTH/4.,-GRAB_HEIGHT/4., 170.);
		GLubyte colors[3][3] = {{255,0,0}, {0,255,0}, {0,0,255}};
		for(j = 0; j<3; j++)
			for(i = 0; i<256; i++)
				if(Histogram[j][i] > max_){
					max_ = (float)Histogram[j][i];
					imax = i;
				}
		max1 = max_;
		max_ /= GRAB_HEIGHT/2.;
		for(j = 0; j<3; j++){
			glColor3ubv(colors[j]);
			glBegin(GL_LINE_STRIP);
			for(i = 0; i<256; i++){
				if(Histogram[j][i] == 0) continue;
				 glVertex3f((float)i-imax+128, (float)Histogram[j][i]/max_, 0);
			}
			glEnd();
		}
		snprintf(str, 127, "%.1f", QMin[HistCoord[0]][HistCoord[1]]);
		renderBitmapString(-GRAB_WIDTH/2.+50, -GRAB_HEIGHT/2., GLUT_BITMAP_HELVETICA_18 , str, white);
		snprintf(str, 127, "%.1f", QMax[HistCoord[0]][HistCoord[1]]);
		renderBitmapString(GRAB_WIDTH/2. - 50., -GRAB_HEIGHT/2., GLUT_BITMAP_HELVETICA_18 , str, white);
		snprintf(str, 127, "max: %.1f, Imax: %.1f", max_, Imax=(HMin + dI * imax));
		renderBitmapString(0, GRAB_HEIGHT/2.-30., GLUT_BITMAP_HELVETICA_18 , str, white);
		free(str);
	}
	glFinish();
	glutSwapBuffers();
	pthread_mutex_unlock(mutex);
}

void Resize(int width, int height){
	int window = glutGetWindow();
	float tmp, wd = (float) width/GRAB_WIDTH, ht = (float)height/GRAB_HEIGHT;
	tmp = (wd+ht)/2.;
	width = (int)(tmp * GRAB_WIDTH); height = (int)(tmp * GRAB_HEIGHT);
	glutReshapeWindow(width, height);
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	GLfloat W = (GLfloat)width; GLfloat H = (GLfloat)height;
	gluPerspective(45., W/H, 0., 100.0);
	gluLookAt(0., 0., GRAB_HEIGHT, 0., 0., 0., 0., 1., 0.);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

