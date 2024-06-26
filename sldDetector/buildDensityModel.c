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
 * DISTAR Case 39809, cleared May 22, 2024 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */
#include "../common/util.h"
#include "defs.h"
#include "sldFileDefs.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define UPPERPERCENT .97
#define LOWERPERCENT .03
double *TIMESAMPLE=0;
double *COUNTSAMPLE=0;
double *DENSITYSAMPLE=0;
size_t SAMPLEX=0;
void getRange(const double l[],size_t n,double *min, double *max, double *absMin,double *absMax){
	*absMin=l[0];
	*absMax=l[n-1];
	size_t i = UPPERPERCENT * n;
	*max = l[i];
	i = LOWERPERCENT * n;
	*min = l[i];
}
void dumpList(double l[],size_t n){
	fprintf(stderr,"LIST:\n");
	for (size_t i=0;i<n;++i){
		fprintf(stderr," %1.6f\n",l[i]);
	}
}
int compDouble(const void *e1, const void *e2){
	double *dp1=(double *)e1;
	double *dp2=(double *)e2;
	if (*dp1<*dp2){return(-1);}
	if (*dp1>*dp2){return(1);}
	return(0);
}
extern void meanStdDev(double *s,int n, double *m, long double *stddev,
		double *max,double *maxV,double *meanV);
#define MAXSAMPLE 2000000
int isSldObs(const struct dirent *d){
	size_t i=sizeof("sldObs.")-1;
	if (strncmp("sldObs.",d->d_name,i)!=0){
		return(0);
	}
	while (d->d_name[i] != '\0'){
		if (isdigit(d->d_name[i]) == 0){return(0);}
		++i;
	}
	return(1);
}
int isSldTSDir(const struct dirent *d){
	size_t i=0;
	while (d->d_name[i] != '\0'){
		if (isdigit(d->d_name[i]) == 0){return(0);}
		++i;
	}
	return(1);
}
bool isFile(char *d){
	struct stat s;
	if (stat(d,&s) < 0){
		fprintf(stderr,"%s.%d stat(%s): %s\n",
			__FUNCTION__,__LINE__,d,strerror(errno));
		return(false);
	}
	if ((s.st_mode & S_IFMT)==S_IFREG){return(true);} //NOLINT(hicpp-signed-bitwise)
	return(false);
}

bool isDir(char *d){
	struct stat s;
	if (stat(d,&s) < 0){
		fprintf(stderr,"%s.%d stat(%s): %s\n",
			__FUNCTION__,__LINE__,d,strerror(errno));
		return(false);
	}
	if ((s.st_mode & S_IFMT)==S_IFDIR){return(true);} //NOLINT(hicpp-signed-bitwise)
	return(false);
}
int getReadings(char *line,double *t,double *c){
	char *s=0;
	char *e=0;
	s=line;
	*t=strtod(s,&e);
	if ((e==line)||(*t==HUGE_VAL)||(*t==-HUGE_VAL)){
		fprintf(stderr,"cannot parse %s\n",line);
		return(-1);
	}
	s=e;
	e=0;
	*c=strtod(s,&e);
	if ((e==line)||(*c==HUGE_VAL)||(*c==-HUGE_VAL)){
		fprintf(stderr,"cannot parse %s\n",line);
		return(-1);
	}
	return(0);
}

static size_t processedFiles=0;
int processSldFile(char *d){
	char b[LONGSTRING];
	FILE *obs=fopen(d,"re");
	if (obs==0){
		fprintf(stderr,"%s.%d fopen(%s) %s\n",
				__FUNCTION__,__LINE__,d,strerror(errno));
		exit(-1);
	}
	long n=fseek(obs,0,SEEK_END);
	if (n < 0){
		fprintf(stderr,"%s.%d fseek(%s) %s\n",
				__FUNCTION__,__LINE__,d,strerror(errno));
		exit(-1);
	}
	n=ftell(obs);
	if (n == 0){
		fprintf(stderr,"%s.%d %s is empty\n",
				__FUNCTION__,__LINE__,d);
		return(-1);
	}
	rewind(obs);
	fprintf(stdout,"%60s %10lu\r",d,processedFiles++);
	TIMESAMPLE=realloc(TIMESAMPLE,(SAMPLEX+n)*sizeof(double));
	COUNTSAMPLE=realloc(COUNTSAMPLE,(SAMPLEX+n)*sizeof(double));
	DENSITYSAMPLE=realloc(DENSITYSAMPLE,(SAMPLEX+n)*sizeof(double));
	while(fgets(b,sizeof(b),obs)){
		if (getReadings(b,&TIMESAMPLE[SAMPLEX],
					&COUNTSAMPLE[SAMPLEX])!=0){
			fprintf(stderr,"%s.%d cannot parse %s\n",
				__FUNCTION__,__LINE__,b);
			continue;
		}
		if (TIMESAMPLE[SAMPLEX] == 0){continue;}
		if (COUNTSAMPLE[SAMPLEX] == 0){continue;}
		DENSITYSAMPLE[SAMPLEX]=COUNTSAMPLE[SAMPLEX]/TIMESAMPLE[SAMPLEX];
		++SAMPLEX;
	}
	return(0);
}
int processSldTSDir(char *d){
	struct dirent **file=0;
	int nFiles=0;
	if ((nFiles=scandir(d,&file,isSldObs,0))<0){
		fprintf(stderr,"%s.%d scandir %s failed with %s\n",
			__FUNCTION__,__LINE__,d,strerror(errno));
		return(-1);
	}
	for(int i=0;i<nFiles;++i){
		char path[PATH_MAX];
		snprintf(path,PATH_MAX,"%s/%s",d,file[i]->d_name);
		if (isFile(path)){
			if (processSldFile(path)!=0){
				continue;
			}
			free(file[i]);
		}
	}
	return(0);
}
int processSldTopDir(char *d){
	struct dirent **file=0;
	int nFiles=0;
	if ((nFiles=scandir(d,&file,isSldTSDir,0))<0){
		fprintf(stderr,"%s.%d scandir %s failed with %s\n",
			__FUNCTION__,__LINE__,d,strerror(errno));
		return(-1);
	}
	for(int i=0;i<nFiles;++i){
		char path[PATH_MAX];
		snprintf(path,PATH_MAX,"%s/%s",d,file[i]->d_name);
		if (isDir(path)){
			if (processSldTSDir(path)!=0){
				return(-1);
			}
			free(file[i]);
		}
	}
	return(0);
}
int main(int argc, char *argv[]){
	double mean=0;
	long double stddev=0;
	double max=0;
	double maxV=0;
	double minV=0;
	double maxP=0;
	double minP=0;
	char obsDir[PATH_MAX];
	char HOST[SHORTSTRING];
	HOST[0]='\0';
	for (size_t i=1;i<argc;++i){
		if (strncmp(argv[i],"--host",strlen("--host"))==0) {
			strncpy(HOST,argv[i+1],PATH_MAX);
			++i;
			continue;
		}
		fprintf(stderr,"%s.%d bad argument %s\n",__FUNCTION__,__LINE__,argv[i]);
		exit(-1);
	}
	if (HOST[0] == '\0') {
		fprintf(stderr,"%s.%d must specify host with --host\n",__FUNCTION__,__LINE__);
		exit(-1);
	}
	fprintf(stdout,"Building SLD Model from Observations\n");
	snprintf(obsDir,sizeof(obsDir),OBSSTORED,HOST);
	processSldTopDir(obsDir);
	if (TIMESAMPLE == 0){
		fprintf(stderr,"%s.%d TIMESAMPLE is empty\n",
				__FUNCTION__,__LINE__);
		exit(-1);
	}
	if (COUNTSAMPLE == 0){
		fprintf(stderr,"%s.%d COUNTSAMPLE is empty\n",
				__FUNCTION__,__LINE__);
		exit(-1);
	}
	if (DENSITYSAMPLE == 0){
		fprintf(stderr,"%s.%d DENSITYSAMPLE is empty\n",
				__FUNCTION__,__LINE__);
		exit(-1);
	}
	qsort(COUNTSAMPLE,SAMPLEX,sizeof(TIMESAMPLE[0]),compDouble);
	qsort(TIMESAMPLE,SAMPLEX,sizeof(TIMESAMPLE[0]),compDouble);
	qsort(DENSITYSAMPLE,SAMPLEX,sizeof(TIMESAMPLE[0]),compDouble);
	getRange(TIMESAMPLE,SAMPLEX,&minP,&maxP,&minV,&maxV);
	if (mkdirEach(SLDMODELDIR)!=0){
		fprintf(stderr,"%s.%d cannot access %s\n",
				__FUNCTION__,__LINE__,SLDMODELDIR);
		exit(-1);
	}
	FILE *modelFile=fopen(SLDMODELS,"w+e");
	if (modelFile==0){
		fprintf(stderr,"%s.%d fopen(%s) %s\n",
				__FUNCTION__,__LINE__,SLDMODELS,strerror(errno));
		exit(-1);
	}
	fprintf(modelFile,"TIME03: %1.6f\nTIME97: %1.6f\nTIME0: %1.6f\nTIME100: %1.6f\n",minP,maxP,minV,maxV);
	getRange(COUNTSAMPLE,SAMPLEX,&minP,&maxP,&minV,&maxV);
	fprintf(modelFile,"COUNT03: %1.6f\nCOUNT97: %1.6f\nCOUNT0: %1.6f\nCOUNT100: %1.6f\n",minP,maxP,minV,maxV);
	getRange(DENSITYSAMPLE,SAMPLEX,&minP,&maxP,&minV,&maxV);
	fprintf(modelFile,"DENSITY03: %1.6f\nDENSITY97: %1.6f\nDENSITY0: %1.6f\nDENSITY100%1.6f\n",minP,maxP,minV,maxV);
	meanStdDev(TIMESAMPLE,SAMPLEX,&mean,&stddev,&max,&maxV,&minV);
	fprintf(modelFile,"TIMEMEAN:%g\nTIMESTDDEV:%Lg\nTIMEMAXDEV: %g\n",mean,stddev,max);
	meanStdDev(COUNTSAMPLE,SAMPLEX,&mean,&stddev,&max,&maxV,&minV);
	fprintf(modelFile,"COUNTMEAN %g\nCOUNTSTD: %Lg\nCOUNTMAXDEV: %g\n",mean,stddev,max);
	meanStdDev(DENSITYSAMPLE,SAMPLEX,&mean,&stddev,&max,&maxV,&minV);
	fprintf(modelFile,"DENSITYMEAN: %g\nDENSITYSTDDEV: %Lg\nDENSITYMAXDEV: %g\n",mean,stddev,max);
	fprintf(stdout,"\n\nSLD Model ready\n");
}
