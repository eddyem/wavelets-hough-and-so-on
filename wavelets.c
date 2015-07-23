// Onur G. Guleryuz 1995, 1996, 1997,
// University of Illinois at Urbana-Champaign,
// Princeton University,
// Polytechnic University.
#include <stdio.h>
#include <stdlib.h>
#include "wavelets.h"
#include "test.h"
// порядка 40мс на изображение 1024х1024
#define check_ptr(ptr,fn) \
	if((ptr)==NULL) { \
		printf("%10s: NULL pointer\n",(char *)(fn)); \
		perror((char *)(fn)); \
		exit(1); \
	}

#ifdef __TEST__
double timesum=0.; int cntr=1; bool firsttime=1;// для среднего времени
#endif

float MLP[2] ={(float)0.70711,(float)0.70711};
float MHP[2] ={(float)0.70711,(float)-0.70711};
int begilp=0, begihp=0, begflp=0, begfhp=0;

WImage alloc_ima(int N,int M,char zero){
	int i;
	WImage mymat = (WImage)malloc(N*sizeof(float *));
	check_ptr(mymat,"allocate_2d_float");
	if(!zero)
		for(i=0;i<N;i++) {
			mymat[i]=(float *)malloc(M*sizeof(float));
			check_ptr(mymat[i],"allocate_2d_float");
	}
	else
		for(i=0;i<N;i++) {
			mymat[i]=(float *)calloc(M,sizeof(float));
			check_ptr(mymat[i],"allocate_2d_float");
	}
	return(mymat);
}

void free_ima(WImage a,int N){
	int i;
	for(i=0;i<N;i++)
		free((void *)a[i]); 
	free((void *)a);
}

float *allocate_1d_float(int N,char zero){
	float *arr;
	if(!zero)
		arr=(float *)malloc(N*sizeof(float));
	else
		arr=(float *)calloc(N,sizeof(float));
	check_ptr(arr,"allocate_1d_float");
	return(arr);
}

void extend_periodic(float *data,int N){
	data[-1] = data[N-1];
	data[-2] = data[N-2];
	data[N] = data[0];
	data[N+1] = data[1];
}

int filter_n_decimate(float *data,float *coef,int N,float *filter,int beg){
	int i,k;
	k=0;
	for(i=beg;i<N+beg;i+=2){
		coef[k] = data[i]*filter[0] + data[i-1]*filter[1];
		k++;
	}
	return(k);
}

void upsample_n_filter(float *coef,float *data,int N,float *filter,int beg){
	int i,l,n,p,fbeg;
	fbeg=1-beg;
	for(i=0;i<N;i++){
		l=0;
		n=(i+fbeg)%2;
		p=(i+fbeg)/2;
		data[i] += coef[p-l] * filter[n];
		l++;
	}
}

void filt_n_dec_all_rows(WImage image,int Ni,int Nj){
	int i,j,ofs;
	float *data, *ptr, *ptr0;
	data=allocate_1d_float((Nj+4),0);
	data+=2;
	ofs=Nj>>1;
	for(i=0;i<Ni;i++){
		ptr = data; ptr0 = image[i];
		for(j=0;j<Nj;j++)
			*ptr++  = *ptr0++;
		extend_periodic(data,Nj);
		filter_n_decimate(data,image[i],Nj,MLP,begflp);
		filter_n_decimate(data,image[i]+ofs,Nj,MHP,begfhp);
	}
	data-=2;
	free((void *)data);
}

void filt_n_dec_all_cols(WImage image,int Ni,int Nj){
	int i,j,ofs;
	float *data,*data2;
	data=allocate_1d_float((Ni+4),0);
	data+=2;
	data2=allocate_1d_float(Ni,0);
	ofs=Ni>>1;
	for(j=0;j<Nj;j++) {
		for(i=0;i<Ni;i++)
			data[i]=image[i][j];
		extend_periodic(data,Ni);
		filter_n_decimate(data,data2,Ni,MLP,begflp);
		filter_n_decimate(data,data2+ofs,Ni,MHP,begfhp);
		for(i=0;i<Ni;i++)
			image[i][j]=data2[i];
	}
	data-=2;
	free((void *)data);
	free((void *)data2);
}

void ups_n_filt_all_rows(WImage image,int Ni,int Nj,int lev,int *shift_arr){
	int i,j,k,ofs;
	float *data1,*data2;
	ofs=Nj>>1;
	data1=allocate_1d_float((ofs+4),0);
	data2=allocate_1d_float((ofs+4),0);
	data1+=2;data2+=2;
	for(i=0;i<Ni;i++) {	
		for(j=0;j<ofs;j++) {
			k=j+ofs;
			data1[j]=image[i][j];image[i][j]=0;
			data2[j]=image[i][k];image[i][k]=0;
		}
		extend_periodic(data1,ofs);	
		extend_periodic(data2,ofs);	
		upsample_n_filter(data1,image[i],Nj,MLP,begilp);
		upsample_n_filter(data2,image[i],Nj,MHP,begihp);
	}
	data1-=2;data2-=2;
	free((void *)data1);
	free((void *)data2);
}

void ups_n_filt_all_cols(WImage image,int Ni,int Nj,int lev,int *shift_arr){
	int i,j,k,ofs;
	float *data1,*data2,*data3;
	ofs=Ni>>1;
	data1=allocate_1d_float((ofs+4),0);
	data2=allocate_1d_float((ofs+4),0);
	data1+=2;data2+=2;
	data3=allocate_1d_float(Ni,0);
	for(j=0;j<Nj;j++) {	
		for(i=0;i<ofs;i++) {
			k=i+ofs;
			data1[i]=image[i][j];
			data2[i]=image[k][j];
			data3[i]=data3[k]=0;
		}
		extend_periodic(data1,ofs);	
		extend_periodic(data2,ofs);	
		upsample_n_filter(data1,data3,Ni,MLP,begilp);
		upsample_n_filter(data2,data3,Ni,MHP,begihp);
		for(i=0;i<Ni;i++)
			image[i][j]=data3[i];
	}
	data1-=2;data2-=2;
	free((void *)data1);
	free((void *)data2);
	free((void *)data3);
}

void wav2d_inpl(WImage image,int Ni,int Nj,int levs,char forw,int *shift_arr_row,int *shift_arr_col){
	int i,dimi,dimj;
	#ifdef __TEST__
	double t0 = dtime();
	#endif
	dimi=(forw)?Ni:(Ni>>(levs-1));
	dimj=(forw)?Nj:(Nj>>(levs-1));
	for(i=0;i<levs;i++) {
		if(forw){
			begflp=shift_arr_row[i];   
			begfhp=-shift_arr_row[i];
			filt_n_dec_all_rows(image,dimi,dimj);
			begflp=shift_arr_col[i];   
			begfhp=-shift_arr_col[i];
			filt_n_dec_all_cols(image,dimi,dimj);
		}
		else {
			begilp=shift_arr_row[levs-1-i];
			begihp=-shift_arr_row[levs-1-i];
			ups_n_filt_all_rows(image,dimi,dimj,levs-1-i,shift_arr_row);
			begilp=shift_arr_col[levs-1-i];
			begihp=-shift_arr_col[levs-1-i];
			ups_n_filt_all_cols(image,dimi,dimj,levs-1-i,shift_arr_col);
		}
		dimi=(forw)?(dimi>>1):(dimi<<1);
		dimj=(forw)?(dimj>>1):(dimj<<1);
	}
	#ifdef __TEST__
	{
	double dt = dtime() - t0;
	if(!firsttime) timesum += dt;
	fprintf(stderr, "\nmkWavedec(), time: %.6f (aver %.6f)\n", dt, timesum/cntr++);
	if(firsttime){cntr = 1; firsttime = 0;}
	}
	#endif
}

