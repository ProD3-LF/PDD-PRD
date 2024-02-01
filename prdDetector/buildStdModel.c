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
prdModel **tmpModel;
int isPRD(const struct dirent *d){

	if (strncmp(d->d_name,"prdObs.",strlen("prdObs."))==0){
		return(1);
	}
	return(0);
}
long long int prdCount[MAXCW*2];
long long int totalCount;
void initCwCount(){
	totalCount=0;
	for(int i=0;i<MAXCW*2;++i){prdCount[i]=0;}
}
void addCwCount(int cw,long long int count,prdModel *m){
	prdCount[cw+MAXCW]+=count;
	totalCount+=count;
	if ((cw+MAXCW)<m->low){
		m->low=cw+MAXCW;
	}
	if ((cw+MAXCW)>m->high){
		m->high=cw+MAXCW;
	}
}
#define MINPRECISION .01
#define TOPERCENT 100
double precision(double d){
	if (d >= MINPRECISION){return(d);}
	return(MINPRECISION);
}
void createTmpModel(prdModel *m){
	for(int i=0;i<MAXCW*2;++i){
		for(int j=0;j<MAXCW*2;++j){m->ratio[i][j]=-1;}
		m->allRatio[i]=-1;
	}
	for(int i=m->low;i<=m->high;++i){
		for(int j=m->low;j<=m->high;++j){
			if ((prdCount[i]!=0) && (prdCount[j]!=0)){
                                double d = precision((double)prdCount[i]/(double)prdCount[j]);
                                d=d*TOPERCENT;
                                d=round(d);
                                m->ratio[i][j] = (int)d;
				if (m->ratio[i][j]==0){
					fprintf(stderr,"%s.%d ratio is ZERO\n",__FUNCTION__,__LINE__);
				}
			} 
			else{
				if ((i==0) && (j==0)){ m->ratio[i][j]=1;}
				else{m->ratio[i][j]=0;}
			}
		}
                double d = precision((double)prdCount[i]/(double)totalCount);
                d=d*TOPERCENT;
                d=round(d);
                m->allRatio[i] = (int)d;
	}
}

int processInFile(char *inDirName,char *inFileName,prdModel *m){
	long long int count=0,inCount=0,outCount=0;
	int cw=0;
        char fn[LONGSTRING],b[LONGSTRING];
        sprintf(fn,"%s/%s",inDirName,inFileName);
        FILE *f=fopen(fn,"re");
        if (f==0){
                fprintf(stderr,"%s.%d fopen(%s) failed with %s\n",
                        __FUNCTION__,__LINE__,fn,strerror(errno));
                return(-1);
        }
	initCwCount();
	while (fgets(b,LONGSTRING,f)!=0){
		if (sscanf(b,"%d %lld\n",&cw,&count)!=2){
                	fprintf(stderr,"%s.%d parse error: %s\n",
                        	__FUNCTION__,__LINE__,b);
                	return(-1);
		}
		if (cw < 0){++inCount;}
		else{++outCount;}
		addCwCount(cw,count,m);
	}
        fclose(f);
	if ((inCount==0) || (outCount==0)){
		fprintf(stderr,"%s.%d rejecting unidirectional %s\n",
				__FUNCTION__,__LINE__,fn);
		initCwCount();
	}
        return(0);
}
long double square(long long int n){
	return((long double)n*(long double)n);
}
void meanStdDev(long long int *s,int n, int *m, int *stddev,int *max){
	int c=0;
	long double stddevd=0;
	long long int sum=0;
	*m=0;
	*max=0;
	for(int i=0;i<n;++i){
		if (s[i]>0){
			sum += s[i];
			++c;
		}
	}
	if (c==0) {
		*m=-1;
		*stddev=0;
		return;
	}
	if (sum == 0) {
		fprintf(stderr,"%s.%d m is 0\n",__FUNCTION__,__LINE__);
		exit(-1);
	}
	sum = sum/c;
	if (sum == 0) {
		fprintf(stderr,"%s.%d sum is 0\n",__FUNCTION__,__LINE__);
		exit(-1);
	}
	if (sum < 0){
		fprintf(stderr,"%s.%d bad sum %lld\n",__FUNCTION__,__LINE__,sum);
		sum=0;
		for(int i=0;i<n;++i){
			if (s[i]>0){
				sum += s[i];
				fprintf(stderr,"%s.%d %d %lld %lld\n",
					__FUNCTION__,__LINE__,i,s[i],sum);
			}
			if (sum < 0){
				fprintf(stderr,"%s.%d BAD AT %d %lld %lld\n",
					__FUNCTION__,__LINE__,i,s[i],sum);
				 exit(-1);
			}
		}
		exit(-1);
	}
	*m=(int)sum;
	if (c==1) {
		*stddev=0;
		return;
	}
	long double t=0;
	for (int i=0;i<n;++i){
		if (s[i] > 0){
			long long int diff = llabs(s[i] - *m);
			long double d=square(diff);
			if (d < 0){
				fprintf(stderr,"%s.%d bad sqare %d\n",
					__FUNCTION__,__LINE__,*stddev);
				exit(-1);
			}
			t += d;
			if (t < 0){
				fprintf(stderr,"%s.%d %Lf %d %Lf\n",
					__FUNCTION__,__LINE__,t,*m,d);
				exit(-1);
			}
			if (diff > *max){*max = (int)diff;}
		}
	}
	stddevd = sqrt((double)(t/(c-1)));
	*stddev = ceil((double)stddevd);
	if (*stddev < 0){
		fprintf(stderr,"%s.%d bad stddev %d\n",__FUNCTION__,__LINE__,*stddev);
		exit(-1);
	}
	if (*m < 0){
		fprintf(stderr,"%s.%d bad mean %d\n",__FUNCTION__,__LINE__,*m);
		exit(-1);
	}
}	
prdStdModel PRDSTDMODEL;
void createStdModel(prdModel *m[], int n){
	PRDSTDMODEL.low=MAXCW*2;
	PRDSTDMODEL.high=0;
	for (int i=0;i<MAXCW*2;++i){
		for (int j=0;j<MAXCW*2;++j){
			PRDSTDMODEL.dist[i][j].mean=-1;
			PRDSTDMODEL.dist[i][j].stddev=0;
		}
	}
	PRDSTDMODEL.low=MAXCW*2;
	PRDSTDMODEL.high=0;
	for(int i=0;i<n;++i){
		if (m[i]->low < PRDSTDMODEL.low){PRDSTDMODEL.low = m[i]->low;}
		if (m[i]->high > PRDSTDMODEL.high){PRDSTDMODEL.high = m[i]->high;}
	}
	long long int *t = malloc(sizeof(long long int)*n);
	if (t==0){
		fprintf(stderr,"%s.%d malloc failure\n",__FUNCTION__,__LINE__);
		exit(-1);
	}
	for(int j=0;j<=2*MAXCW;++j){
		for(int k=0;k<=2*MAXCW;++k){
			for(int i=0;i<n;++i){
				t[i]=(long long) (m[i]->ratio[j][k]);
			}
			meanStdDev(t,n,&PRDSTDMODEL.dist[j][k].mean,
				&PRDSTDMODEL.dist[j][k].stddev,
				&PRDSTDMODEL.dist[j][k].max);
			if (PRDSTDMODEL.dist[j][k].mean != -1){
				 printf("%d/%d: %d %d %d\n",
					j-MAXCW,k-MAXCW,
					PRDSTDMODEL.dist[j][k].mean,
					PRDSTDMODEL.dist[j][k].stddev,
					PRDSTDMODEL.dist[j][k].max);
			}
		}
		fprintf(stderr,"allRatio[%d]:\n",j);
		for(int i=0;i<n;++i){
			t[i]=(long long) (m[i]->allRatio[j]);
		}
		meanStdDev(t,n,&PRDSTDMODEL.allDist[j].mean,
				&PRDSTDMODEL.allDist[j].stddev,
				&PRDSTDMODEL.allDist[j].max);
		if (PRDSTDMODEL.allDist[j].mean != -1){
			 printf("ALL %d: %d %d %d\n",
				j-MAXCW,
				PRDSTDMODEL.allDist[j].mean,
				PRDSTDMODEL.allDist[j].stddev,
				PRDSTDMODEL.allDist[j].max);
		}
	}
	fprintf(stdout,"%s.%d opening %s\n",__FUNCTION__,__LINE__,STOREDSTDMODELFILE);
	fflush(stdout);
	FILE *modelFile=fopen(STOREDSTDMODELFILE,"we");
        if (modelFile==0){
                fprintf(stderr,"%s.%d fopen(%s) failed with %s\n",
                        __FUNCTION__,__LINE__,STOREDSTDMODELFILE,strerror(errno));
                return;
        }
	fprintf(stdout,"%s.%d writing %s\n",__FUNCTION__,__LINE__,STOREDSTDMODELFILE);
	fflush(stdout);
        if (fwrite(&PRDSTDMODEL,sizeof(prdStdModel),1,modelFile) != 1){
                fprintf(stderr,"%s.%d fwrite() %s\n",
                        __FUNCTION__,__LINE__,strerror(errno));
        }
	fprintf(stdout,"%s.%d closing %s\n",__FUNCTION__,__LINE__,STOREDSTDMODELFILE);
	fflush(stdout);
        fclose(modelFile);

}
int processInDir(char *inDirName){
	struct dirent **file=0;
	int nFiles=scandir(inDirName,&file,isPRD,0);
	if (nFiles<0){
		fprintf(stderr,"%s.%d scandir %s failed with %s\n",
			__FUNCTION__,__LINE__,inDirName,strerror(errno));
		return(-1);
	}
	if (nFiles > MAXFILES){
		fprintf(stderr,"%s.%d sampling %d of %d\n",
			__FUNCTION__,__LINE__,MAXFILES,nFiles);
		 nFiles=MAXFILES;
	}
	if ((tmpModel=malloc(sizeof(prdModel *)*nFiles))==0){
		fprintf(stderr,"%s.%d malloc failed\n",
			__FUNCTION__,__LINE__);
		return(-1);
	}
	for(int i=0;i<nFiles;++i){
		if ((tmpModel[i]=malloc(sizeof(prdModel)))==0){
			fprintf(stderr,"%s.%d malloc failed %d of %d\n",
				__FUNCTION__,__LINE__,i,nFiles);
			return(-1);
		}
	}
	for(int i=0;i<nFiles;++i){
		tmpModel[i]->low=2*MAXCW;
		tmpModel[i]->high=0;
		processInFile(inDirName,file[i]->d_name,tmpModel[i]);
		createTmpModel(tmpModel[i]);
		free(file[i]);
	}
	createStdModel(tmpModel,nFiles);
	return(0);
}

int main(){
	processInDir(TMPWORKINGDIR);
}

