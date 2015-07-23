#include "wavedec.h"
#include "test.h"
#include "events.h"
#include "wavelets.h"

WImage imageR = NULL, imageG = NULL, imageB = NULL;

float min_, max_, QMax[2][2], QMin[2][2], Widths[2][2], Areas[2][2];
int wImageSize;
GLubyte *WaveletBits = NULL;
GLubyte *WVLT = NULL;
int **Hough = NULL, Rmax=0;
int *Histogram[3] = {NULL, NULL, NULL};
float HMin, dI;
int HistCoord[2] = {1, 1};
int more_info = 0;
int SinBufSize = 0;
double *SinBuf = NULL;
double *CosBuf = NULL;
double degStep = 1.;
double conv;

void fillSinBuf(){ // заполнение таблицы синусов
	int i;
	conv = M_PI / 180.;
	double degr = -M_PI/2, step = degStep * conv;
	SinBufSize = (int)(180./degStep);
	SinBuf = (double*) calloc(SinBufSize, sizeof(double));
	CosBuf = (double*) calloc(SinBufSize, sizeof(double));
	for(i = 0; i < SinBufSize; i ++){
		SinBuf[i] = sin(degr);
		CosBuf[i] = cos(degr);
		degr += step;
	}
}

void alloc_mem(){
	int size, i, j;
	if(imageR) free_ima(imageR, wImageSize); 
	if(imageG) free_ima(imageG, wImageSize);
	if(imageB) free_ima(imageB, wImageSize);
//	if(WaveletBits) free(WaveletBits); 
	wImageSize = (GRAB_WIDTH > GRAB_HEIGHT) ? GRAB_WIDTH : GRAB_HEIGHT;
	i = 0; j = wImageSize;
	while(j>>=1) i++; // получаем ближайшую к wImageSize степень 2 снизу
	if(wImageSize != 1<<i) wImageSize = 1<<(i+1);
//	size = wImageSize * wImageSize;
	size = GRAB_WIDTH * GRAB_HEIGHT * 3;
	imageR = alloc_ima(wImageSize, wImageSize, 1);
	imageG = alloc_ima(wImageSize, wImageSize, 1);
	imageB = alloc_ima(wImageSize, wImageSize, 1);
	if(!WaveletBits) WaveletBits = (GLubyte *)calloc(3*size, sizeof(GLubyte));
	if(!WVLT) WVLT = (GLubyte *)calloc(3*size, sizeof(GLubyte));
	if(Hough){
		for(i = 0; i < SinBufSize; i++)
			free(Hough[i]);
		free(Hough);
	}
	Rmax = 1+(int)(sqrt(GRAB_HEIGHT*GRAB_HEIGHT + GRAB_WIDTH*GRAB_WIDTH));
	Hough = (int**)malloc(SinBufSize * sizeof(int*));
	for(i = 0; i < SinBufSize; i++)
		Hough[i] = (int*)calloc(Rmax, sizeof(int));
		// преобразования Хафа будем делать с шагом 1 градус по тета, 1 по R
	
//	WARN("init memory for wavelets, ext size=%d\n", wImageSize);	
}
/*
void emptyMem(){
	int i,j;
	for(i=0; i<wImageSize; i++)
		for(j=0; j<wImageSize; j++)
			imageR[j][i]=imageG[j][i]=imageB[j][i] = 0.;
}*/

void initWavelet(bool add){ // инициализация вейвлет-пространства и изображения для в.анализа
	int i, j, k, wd = GRAB_HEIGHT+1, stx = 1, sty = 1;
	float *ptr[3];
//	if(!(WaveletBits && imageR && imageG && imageB))
	if(!add) alloc_mem();
//	emptyMem();
	GLubyte *bytes = BitmapBits;
	if(GRAB_WIDTH == wImageSize) stx = 0;
	if(GRAB_HEIGHT == wImageSize){ sty = 0; wd--; }
	if(!add) // инициализируем
		for(j = sty; j < wd; j++){
			ptr[0] = &imageR[j][stx]; ptr[1] = &imageG[j][stx]; ptr[2] = &imageB[j][stx];
			for(i = 0; i < GRAB_WIDTH; i++){
				for(k = 0; k < 3; k++)
					*(ptr[k]++) = (float)(*bytes++);
			}
		}
	else // добавляем
		for(j = sty; j < wd; j++){
			ptr[0] = &imageR[j][stx]; ptr[1] = &imageG[j][stx]; ptr[2] = &imageB[j][stx];
			for(i = 0; i < GRAB_WIDTH; i++){
				for(k = 0; k < 3; k++)
					*(ptr[k]++) += (float)(*bytes++);
			}
		}
/*	fprintf(stderr, "Массив для вейвлетов инициализирован. размеры: %dx%d (увелич.: %d)\n",
		GRAB_WIDTH, GRAB_HEIGHT, wImageSize);*/
}

void mkWavedec(){
	float *ptr[3], wdth, Max = -1e30, Min = 1e30, scale;
	double tt, *cosptr, *sinptr, ii, jj;
	int j, i, k, N, Rm=-1, Tm=-1, shift_arr_r[MAX_ARR_SIZE],shift_arr_c[MAX_ARR_SIZE];
	int halfSize =  wImageSize/2;
	GLubyte *GLptr;
	const int levs=1;
	inline void findminmax(WImage image, char flag){
		float *ptr, pixval;
		int w2 = GRAB_WIDTH/2-2, h2 = GRAB_HEIGHT/2, s2 = wImageSize/2;
		int XLb = w2, XHb = s2+w2, YHb = s2+h2;
		int i, j, px, py;
		s2++;
		if(flag){
			min_ = 1e30; max_ = -1e30;
			for(i=0; i<2; i++) for(j=0; j<2; j++){
				QMax[i][j] = -1e30; QMin[i][j] = 1e30;
				Areas[i][j] = 0.;
			}
		}
		//image += YLb;
		for(j=0; j<YHb; j++, image++){
			if(j < h2) py = 0;
			else if(j > s2) py = 1;
			else continue;
			ptr = *image;
			for(i=0; i<XHb; i++, ptr++){
				if(i < XLb) px = 0;
				else if(i > s2) px = 1;
				else continue;
				pixval = *ptr;
				if(min_ > pixval) min_ = pixval;
				else if(max_ < pixval) max_ = pixval;
				if(QMin[py][px] > pixval) QMin[py][px] = pixval;
				else if(QMax[py][px] < pixval) QMax[py][px] = pixval;
				Areas[py][px] += fabs(pixval);
			}
		}
	}
	inline GLubyte testpix(int pix){
		int res = pix; if(res < 0) res = 0; else if(res > 255) res = 255;
		return (GLubyte)res;
	}
	inline void fillbuf(GLubyte *bytes){ // формирование изображения из компонент
		bool flagy, flagx;
		int sx = GRAB_WIDTH/2+1, sy = GRAB_HEIGHT/2+1, w2 = wImageSize/2-1,
			hx = w2 + sx, hy = w2 + sy, stx = 1, sty = 1;
		if(GRAB_WIDTH == wImageSize){ stx = 0; hx--;}
		if(GRAB_HEIGHT == wImageSize){ sty = 0; hy--;}
		for(j = sty; j < hy; j++){
			ptr[0] = &imageR[j][stx]; ptr[1] = &imageG[j][stx]; ptr[2] = &imageB[j][stx];
			flagy = (j < sy) || (j > w2);
			for(i = stx; i < hx; i++){
				flagx = (i < sx) || (i > w2);
				scale = 255./Widths[j>sy][i>sx];
				for(k = 0; k < 3; k++){
					if(flagx & flagy)
						*bytes++ = (GLubyte) ((*ptr[k] - QMin[j>sy][i>sx]) * scale);
					ptr[k]++;
				}
			}
		}
	}
//	initWavelet();
	for(i=levs-1; i>=0; i--)
		shift_arr_r[i]=shift_arr_c[i]=0;
	wav2d_inpl(imageR,wImageSize,wImageSize,levs,1,shift_arr_r,shift_arr_c);
	wav2d_inpl(imageG,wImageSize,wImageSize,levs,1,shift_arr_r,shift_arr_c);
	wav2d_inpl(imageB,wImageSize,wImageSize,levs,1,shift_arr_r,shift_arr_c);
	
	findminmax(imageR, 1);
	findminmax(imageG, 0);
	findminmax(imageB, 0);
	wdth = 255. / (max_ - min_);
	// Статистика
	for(j=0; j<2; j++) for(i=0; i<2; i++)
		Widths[j][i] = QMax[j][i] - QMin[j][i];
	if(more_info){
		fprintf(stderr, "max=%.3g, min=%.3g\n", max_, min_);
		fprintf(stderr, "Экстремумы по квадрантам (min, max):\n");
		fprintf(stderr, "(%.3g, %.3g)\t\t\t(%.3g, %.3g)\n", QMin[0][0], QMax[0][0],
			QMin[0][1], QMax[0][1]);
		fprintf(stderr, "(%.3g, %.3g)\t\t\t(%.3g, %.3g)\n", QMin[1][0], QMax[1][0],
			QMin[1][1], QMax[1][1]);
	}
	fillbuf(WaveletBits); // заполняем WaveletBits, обрезая лишние поля

	// удаляем LL и HH
	for(i = 0; i < halfSize; i++){
		for(j = 0; j < halfSize; j++)
			imageR[i][j] = imageB[i][j] = imageG[i][j] = 0.;
		}
	for(i = halfSize; i < wImageSize; i++){
		for(j = halfSize; j < wImageSize; j++)
			imageR[i][j] = imageB[i][j] = imageG[i][j] = 0.;
		}
	wav2d_inpl(imageR,wImageSize,wImageSize,levs,0,shift_arr_r,shift_arr_c);
	wav2d_inpl(imageG,wImageSize,wImageSize,levs,0,shift_arr_r,shift_arr_c);
	wav2d_inpl(imageB,wImageSize,wImageSize,levs,0,shift_arr_r,shift_arr_c);
	for(i = 0; i < GRAB_HEIGHT; i++){
		ptr[0] = imageR[i]; ptr[1] = imageG[i]; ptr[2] = imageB[i];
		for(j = 0; j < GRAB_WIDTH; j++)
			for(k = 0; k < 3; k++){
				if(*ptr[k] > Max) Max = *ptr[k];
				if(*ptr[k] < Min) Min = *ptr[k];
				ptr[k]++;
			}
	}
	scale = 255. / Max;
	Max *= .2;
	int cntr = 0;
	// и заполняем WVLT
	GLptr = WVLT;
	double t0 = dtime();
	for(i = 0, ii = 0.; i < GRAB_HEIGHT; i++, ii += 1.){
		ptr[0] = imageR[i]; ptr[1] = imageG[i]; ptr[2] = imageB[i];
		for(j = 0, jj = 0.; j < GRAB_WIDTH; j++, jj += 1.){
			N = 0;
			for(k = 0; k < 3; k++){
				if((wdth = *ptr[k]) > Max){
					*GLptr++ = (GLubyte) (*ptr[k] * scale );
					N++;
				}
				else *GLptr++ = 0;
				ptr[k]++;
				}
			if(N == 3){
				cntr++;
				cosptr = CosBuf; sinptr = SinBuf;
				for(k=0; k<SinBufSize; k++){
					N = (int)(jj * (*cosptr++) + ii * (*sinptr++));
					if(N > 0 && N < Rmax)
						Hough[k][N]++;					
				}
			}
		}
	}
	N = 0;
	for(i = 0; i < Rmax; i++)
		for(j = 0; j < SinBufSize; j++)
			if(Hough[j][i] > N){
				N = Hough[j][i];
				Rm = i;
				Tm = j;
			}
	fprintf(stderr, "%.6f секунд\n", dtime() - t0);
	j = (double)Rm/SinBuf[Tm];
	tt = CosBuf[Tm] / SinBuf[Tm];
	for(i=0; i<GRAB_WIDTH; i++){
		k = j - (double)i*tt;
		if(k>0 && k < GRAB_HEIGHT){
			GLptr = &WVLT[(k*GRAB_WIDTH + i)*3];
			*GLptr++ = 255; *GLptr++ = 0; *GLptr++ = 0;
		}
	}
	fprintf(stderr, "%d точек; прямая R=%d, theta=%g, N=%d\n", cntr, Rm, (double)Tm * degStep - 90., N);
}

void buildHistogram(){ // построение гистограммы по заданному вейвлет-образу
	int w2 = GRAB_WIDTH/2-2, h2 = GRAB_HEIGHT/2-2, s2 = wImageSize/2;
	int XHb = s2+w2, YHb = s2+h2;
	int x, y, k, x0, y0, intens;
	float wd;
	float *ptr[3];
	if(Histogram[0]){free(Histogram[0]);free(Histogram[1]);free(Histogram[2]);}
	for(x=0; x<3; x++)
		Histogram[x] = (int*) calloc(260, sizeof(float));
	x0 = s2+2;
	y0 = s2+2;
	wd = Widths[HistCoord[0]][HistCoord[1]];
	HMin = QMin[HistCoord[0]][HistCoord[1]];
//	dI = (wd>255.) ? wd / 255. : 1.;
	dI = wd/255.;
	if(fabs(dI) < 0.0001) return; // ширина гистограммы == 0
	if(HistCoord[0] == 0){
		x0 = 2;
		XHb = w2;
	}
	if(HistCoord[1] == 0){
		y0 = 2;
		YHb = h2;
	}
	for(y=y0; y<YHb; y++){
		ptr[0] = &imageR[y][x0]; ptr[1] = &imageG[y][x0]; ptr[2] = &imageB[y][x0];
		for(x=x0; x<XHb; x++){
			for(k=0; k<3; k++){
				intens = (int)((*ptr[k]++ - HMin) / dI);
				if(intens < 256 && intens > -1)  Histogram[k][intens]++;
			}
		}
	}
	
}




