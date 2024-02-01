/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2012-2023 Applied Communication Sciences
 * (now Peraton Labs Inc.)
 *
 * This software was developed in work supported by the following U.S.
 * Government contracts:
 *
 * HR0011-20-C-0160 HR0011-16-C-0061
 * 
 *
 * Any opinions, findings and conclusions or recommendations expressed in
 * this material are those of the author(s) and do not necessarily reflect
 * the views, either expressed or implied, of the U.S. Government.
 *
 * DoD Distribution Statement A
 * Approved for Public Release, Distribution Unlimited
 *
 * DISTAR Case 38846, cleared November 1, 2023
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */
#include "../common/flags.h"
#include "../common/prdObs.h"
#include "defs.h"
#include "prdFileDefs.h"
#include "prdModel.h"
#include "prdStdModel.h"
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
long long COUNT[MAXFILES][MAXCW*2];
prdModel **tmpModel;
long long int low=MAXCW*2,high=0;
int isPRD(const struct dirent *d){
	if (strncmp(d->d_name,"prdObs.",strlen("prdObs."))==0){
		return(1);
	}
	return(0);
}
void initCwCount(){
	for (int k=0;k<MAXFILES;++k){for(int i=0;i<MAXCW*2;++i){COUNT[k][i]=0;}}
}
void addCwCount(int fileNo,long long int cw,long long int count){
	if (count==0){return;}
	if ((cw+MAXCW) > high){high = cw+MAXCW+1;}
	if ((cw+MAXCW) < low){low = cw+MAXCW;}
	COUNT[fileNo][cw+MAXCW]+=count;
}
int processInFile(char *inDirName,char *inFileName){
        char b[PATH_MAX];
	static int fileNo=0;
        sprintf(b,"%s/%s",inDirName,inFileName);
        FILE *f=fopen(b,"re");
        if (f==0){
                fprintf(stderr,"%s.%d fopen(%s) failed with %s\n",
                        __FUNCTION__,__LINE__,b,strerror(errno));
                return(-1);
        }
	while (fgets(b,sizeof(b),f)!=0){
		long long int count=0,cw=0;
		if (sscanf(b,"%lld %lld\n",&cw,&count)!=2){
                	fprintf(stderr,"%s.%d parse error: %s\n",
                        	__FUNCTION__,__LINE__,b);
                	return(-1);
		}
		addCwCount(fileNo,cw,count);
	}
        fclose(f);
	++fileNo;
        return(0);
}
int processInDir(char *inDirName){
	struct dirent **file=0;
	int nFiles=scandir(inDirName,&file,isPRD,0);
	if (nFiles<0){
		fprintf(stderr,"%s.%d scandir failed with %s\n",
			__FUNCTION__,__LINE__,strerror(errno));
		return(-1);
	}
	if (nFiles > MAXFILES){
		fprintf(stderr,"%s.%d sampling %d of %d\n",
			__FUNCTION__,__LINE__,MAXFILES,nFiles);
		 nFiles=MAXFILES;
	}
	for(int i=0;i<nFiles;++i){
		processInFile(inDirName,file[i]->d_name);
		free(file[i]);
		fprintf(stderr,"%s.%d file %d/%d\n",
				__FUNCTION__,__LINE__,i,nFiles);
	}
	return(nFiles);
}

long double corcof(const long long x[],const long long y[],int n) {
    long double nr=0,dr_1=0,dr_2=0,dr_3=0,dr=0;
    long double *xx=0,*xy=0,*yy=0;
    long double sum_y=0,sum_yy=0,sum_xy=0,sum_x=0,sum_xx=0;
    int i=0;
    xx=malloc(n*sizeof(long double));
    xy=malloc(n*sizeof(long double));
    yy=malloc(n*sizeof(long double));
    for(i=0;i<n;i++) {
     xx[i]=x[i]*x[i];
     yy[i]=y[i]*y[i];
    }
    for(i=0;i<n;i++) {
     sum_x+=x[i];
     sum_y+=y[i];
     sum_xx+= xx[i];
     sum_yy+=yy[i];
     sum_xy+= x[i]*y[i];     
    }
    nr=(n*sum_xy)-(sum_x*sum_y);
    long double sum_x2=sum_x*sum_x;
    long double sum_y2=sum_y*sum_y;
    dr_1=(n*sum_xx)-sum_x2;
    dr_2=(n*sum_yy)-sum_y2;
    dr_3=dr_1*dr_2;
    dr=sqrt(dr_3);
    free(xx);
    free(xy);
    free(yy);
    if (dr==0){
	return(0);
    }
    return(nr/dr);
}
int C[MAXCW*2][MAXCW*2];
int allZeroCol(int c,int l,int h){
	for (int i=l;i<h;++i){if (C[i][c]!=0){return(1);}}
	return(0);
}
int allZeroRow(int r,int l,int h){
	for (int i=l;i<h;++i){if (C[r][i]!=0){return(1);}}
	return(0);
}
void dumpModel(int start,int stop){
	int lowC=MAXCW*2,highC=0,lowR=MAXCW*2,highR=0;
	FILE *f=fopen(STOREDCORCOFFILE,"re");
	if (f==0){
		fprintf(stderr,"%s.%d fopen failed with: %s\n",
			__FUNCTION__,__LINE__,strerror(errno));
		exit(-1);
	}
	if (fread(C,MAXCW*2*MAXCW*2*sizeof(int),1,f) !=1){
		fprintf(stderr,"%s.%d fread failed with: %s\n",
			__FUNCTION__,__LINE__,strerror(errno));
		exit(-1);
	}
	fclose(f);
	for(int k=0;k<MAXCW*2;++k){
		for(int j=0;j<MAXCW*2;++j){
			if (C[k][j]!=0){
				if (k<lowC){lowC=k;}
				if (j<lowR){lowR=j;}
				if (j>highR){highR=j;}
				if (k>highC){highC=k;}
			}
		}
	}
	++highC;
	++highR;
	for(int k=lowC;k<highC;++k){
		if (allZeroCol(k,lowR,highR)==0){continue;}
		if (inRequestedRange(start,stop,k) == 1){continue;}
		//fprintf(stderr,",%s",convert(k-MAXCW));
		fprintf(stderr,",%d",(k-MAXCW));
	}
	fprintf(stderr,"\n");
	for(int k=lowC;k<highC;++k){
		if (allZeroCol(k,lowR,highR)==0){continue;}
		if (inRequestedRange(start,stop,k) == 1){continue;}
		fprintf(stderr,"%d",(k-MAXCW));
		for(int j=lowR;j<highR;++j){
			if (allZeroRow(j,lowC,highC)==0){continue;}
			if (inRequestedRange(start,stop,j) == 1){continue;}
			fprintf(stderr,",%d",C[k][j]);
		}
		fprintf(stderr,"\n");
	}
}
void outputModel(){
	FILE *f=fopen(STOREDCORCOFFILE,"wb+e");
	if (f==0){
		fprintf(stderr,"%s.%d fopen(%s) failed with: %s\n",
			__FUNCTION__,__LINE__,STOREDCORCOFFILE,strerror(errno));
		exit(-1);
	}
	if (fwrite(C,MAXCW*2*MAXCW*2*sizeof(int),1,f) !=1){
		fprintf(stderr,"%s.%d fwrite failed with: %s\n",
			__FUNCTION__,__LINE__,strerror(errno));
		exit(-1);
	}
	fclose(f);
}
#define TOPERCENT 100
void calCor(int nFiles){
	long long X[MAXFILES],Y[MAXFILES];
	for(long long k=low;k<high;++k){
		for(long long j=low;j<high;++j){
			for (int i=0;i<nFiles;++i){
				X[i]=COUNT[i][k];
				Y[i]=COUNT[i][j];
			}
			C[k][j]=(int)(TOPERCENT*corcof(X,Y,nFiles));
		}
	}
}
int main(int argc,char *arv[]){
	char *e=initCwMap();
	if (e!=0){
		fprintf(stderr,"%s.%d initCwMap(): %s\n",__FUNCTION__,__LINE__,e);
		return(-1);
	}
	if (argc>1){
		if (strcmp(arv[1],"--dump")==0) {
			int start=0,stop=MAXCW;
			if (argc == 4){
				start=atoi(arv[2]);
				stop=atoi(arv[3]);
				if (start > stop) {
					fprintf(stderr,"%s.%d stop must be > start\n",__FUNCTION__,__LINE__);
					return(-1);
				}
			}
			dumpModel(start,stop);
			exit(0);
		}
		else {
			fprintf(stderr,"%s.%d bad arg %s\n",
				__FUNCTION__,__LINE__,arv[1]);
			exit(-1);
		}
	}
	initCwCount();
	int nFiles=processInDir(TMPWORKINGDIR);
	calCor(nFiles);
	outputModel();
}
