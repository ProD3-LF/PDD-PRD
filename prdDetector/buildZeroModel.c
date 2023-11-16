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
#include "zProb.h"
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
int minCW=MAXCW*2;
int maxCW=0;
zProb PZ[MAXCW*2];
void outputModel(){
	FILE *f=fopen(STOREDZPROBFILE,"wbe");
	if (f==0){
		fprintf(stderr,"%s.%d fopen failed with: %s\n",
			__FUNCTION__,__LINE__,strerror(errno));
		exit(-1);
	}
	if (fwrite(PZ,MAXCW*2*sizeof(zProb),1,f) !=1){
		fprintf(stderr,"%s.%d fwrite failed with: %s\n",
			__FUNCTION__,__LINE__,strerror(errno));
		exit(-1);
	}
	fclose(f);
}
int isPRD(const struct dirent *d){

	if (strncmp(d->d_name,"prdObs.",strlen("prdObs."))==0){
		return(1);
	}
	return(0);
}
void initZProb(){
	for(int i=0;i<MAXCW*2;++i){
		for (int j=0;j<MAXCW*2;++j){
			 PZ[i].pZeroHigh[j]=
				PZ[i].pZeroMed[j] =
					PZ[i].pZeroLow[j] = 0;
		}
	}
}
long long int rawObs[MAXFILES][MAXCW*2];
void initRawObs(){
	for(int i=0;i<MAXFILES;++i){
		for(int j=0;j<MAXCW*2;++j){
			rawObs[i][j]=0;
		}
	}
}
static int fileNo=0;
void dumpModel(){
	FILE *f=fopen(STOREDZPROBFILE,"re");
	if (f==0){
		fprintf(stderr,"%s.%d fopen failed with: %s\n",
			__FUNCTION__,__LINE__,strerror(errno));
		exit(-1);
	}
	if (fread(PZ,MAXCW*2*sizeof(zProb),1,f) !=1){
		fprintf(stderr,"%s.%d fread failed with: %s\n",
			__FUNCTION__,__LINE__,strerror(errno));
		exit(-1);
	}
	fclose(f);
	for(int i=0;i<MAXCW*2;++i){
		fprintf(stdout,"----------ZMODEL FOR %d----------------\n",
			i-MAXCW);
		fprintf(stdout,"med %d, low %d\n",PZ[i].med,PZ[i].low);
		for(int j=0;j<MAXCW*2;++j){
			if ((PZ[i].pZeroHigh[j]==0) &&
				(PZ[i].pZeroMed[j]==0) &&
				(PZ[i].pZeroLow[j]==0)){continue;}
			fprintf(stdout,"%d %d %d %d\n",
				j-MAXCW,PZ[i].pZeroHigh[j],
				PZ[i].pZeroMed[j],
				PZ[i].pZeroLow[j]);
		}
	}
}
void processRawObs(){
	for(int i=minCW;i<maxCW;++i){
		long long int max=0;
		for (int n=0;n<fileNo;++n){
			if (rawObs[n][i]>max){max=rawObs[n][i];}
		}
		PZ[i].low=(int)round(((double)max/(double)3));
		PZ[i].med=PZ[i].low*2;
	}
	for(int i=minCW;i<maxCW;++i){
		for (int ni=0;ni<fileNo;++ni){
			if (rawObs[ni][i]==-1){continue;}
			if (rawObs[ni][i]==0){
				for(int j=minCW;j<maxCW;++j){
					if (rawObs[ni][j]==-1){continue;}
					if (rawObs[ni][j] <= PZ[j].low) {
						++PZ[i].pZeroLow[j];
						continue;
					}
					if (rawObs[ni][j] <= PZ[j].med) {
						++PZ[i].pZeroMed[j];
						continue;
					}
					++PZ[i].pZeroHigh[j];
				}
			}
		}
	}
	for(int i=minCW;i<maxCW;++i){
		for(int j=minCW;j<maxCW;++j){
			int s=0;
			if ((s=PZ[i].pZeroLow[j]+PZ[i].pZeroMed[j]+
				 	PZ[i].pZeroHigh[j])==0){
				PZ[i].pZeroLow[j]=100;
				continue;
			}
			PZ[i].pZeroHigh[j]*=100;
			PZ[i].pZeroMed[j]*=100;
			PZ[i].pZeroLow[j]*=100;
			PZ[i].pZeroHigh[j]=(int)(round((double)PZ[i].pZeroHigh[j]/(double)s));
			PZ[i].pZeroMed[j]=(int)(round((double)PZ[i].pZeroMed[j]/(double)s));
			PZ[i].pZeroLow[j]=(int)(round((double)PZ[i].pZeroLow[j]/(double)s));
			if ((s=PZ[i].pZeroLow[j]+PZ[i].pZeroMed[j]+
				 	PZ[i].pZeroHigh[j])<100){
				fprintf(stderr,"%s.%d s=%d for %d %d\n",
					__FUNCTION__,__LINE__,
					s,i-MAXCW,j-MAXCW);
			}
		}
	}
}
int processInFile(char *inDirName,char *inFileName){
	char b[LONGSTRING];
	sprintf(b,"%s/%s",inDirName,inFileName);
	FILE *f=fopen(b,"re");
	if (f==0){
		fprintf(stderr,"%s.%d fopen(%s) failed with %s\n",
			__FUNCTION__,__LINE__,b,strerror(errno));
		return(-1);
	}
	while (fgets(b,LONGSTRING,f)!=0){
		long long int count=0;
		int cw=0;
		if (sscanf(b,"%d %lld\n",&cw,&count)!=2){
			fprintf(stderr,"%s.%d parse error: %s\n",
				__FUNCTION__,__LINE__,b);
			return(-1);
		}
		cw+=MAXCW;
		rawObs[fileNo][cw]+=count;
		if (cw > maxCW){maxCW=cw;}
		if (cw < minCW){minCW=cw;}
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
		fprintf(stderr,"%s.%d file %d/%d\r",__FUNCTION__,__LINE__,i,nFiles);
	}
	return(nFiles);
}

int main(int argc,char *arv[]){
	initZProb();
	initRawObs();
	char *e=initCwMap();
	if (e!=0){
		fprintf(stderr,"%s.%d initCwMap(): %s\n",__FUNCTION__,__LINE__,e);
		return(-1);
	}
	if (argc>1){
		if (strcmp(arv[1],"--dump")==0) {
			dumpModel();
			exit(0);
		}
		else {
			fprintf(stderr,"%s.%d bad arg %s\n",
				__FUNCTION__,__LINE__,arv[1]);
			exit(-1);
		}
	}
	processInDir(TMPWORKINGDIR);
	processRawObs();
	outputModel();
}
