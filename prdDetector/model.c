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
#include "../common/logMessage.h"
#include "defs.h"
#include "prdFileDefs.h"
#include "prdModel.h"
#include "prdStdModel.h"
#include "zProb.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
int coreCoefficient[MAXCW*2][MAXCW*2];
prdStdModel PRDSTDMODEL;
zProb PZ[MAXCW*2];
int maxCw=0,minCw=MAXCW*2;
void logZeroAnomaly(long long unsigned int now,char *name,int cw1,
		int cw2,int zProb,unsigned int cw2Count,int tresh){
	char cw1s[SHORTSTRING],cw2s[SHORTSTRING];
	convertCw(cw1,cw1s,SHORTSTRING);
	convertCw(cw2,cw2s,SHORTSTRING);
	logMessage(stderr,__FUNCTION__,__LINE__,
		"%lld %s ZERO %s %s model(zProb %d) current(cw2Count %d tresh %d)",
		now,name,cw1s,cw2s,zProb,cw2Count,tresh);
}
void logAnomaly(long long unsigned int now,char * type, char *name,int cw1,
		int cw2,int modelMax,int modelMean,int modelStdDev,
		int curRatio,int dev){
	char cw1s[SHORTSTRING],cw2s[SHORTSTRING];
	convertCw(cw1,cw1s,SHORTSTRING);
	convertCw(cw2,cw2s,SHORTSTRING);
	logMessage(stderr,__FUNCTION__,__LINE__,
		"%lld %s %s %s/%s model(maxdev %d mean %d stddev %d) current(ratio %d dev %d)\n",
		now,name,type,cw1s,cw2s,modelMax,modelMean,modelStdDev,curRatio,dev);
}
int xlate(int cw,unsigned int value){
	if (value <= PZ[cw].low){return(LOW);}
	if (value <= PZ[cw].med){return(MED);}
	return(HIGH);
}

//returns the probability of cw having value cw2Value give cw1 is 0
int checkZeroProb(int cw1,int cw2,unsigned int cw2Value){
	switch(xlate(cw2,cw2Value)){
	case LOW: return(PZ[cw1].pZeroLow[cw2]);
	case MED: return(PZ[cw1].pZeroMed[cw2]);
	case HIGH: return(PZ[cw1].pZeroHigh[cw2]);
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"bad xlate for %d %d\n",cw1,cw2);
	return(HIGH);
}
unsigned long long int totAnom=0;
uint64_t comparCurToModel(char *name,unsigned int prdCount[],prdModel *curState,
		uint64_t now,uint64_t *zCount,uint64_t *errorCount,
		uint64_t *max,uint64_t *two,uint64_t *allmax,
		uint64_t *alltwo){
	uint64_t deviantCount=0;
	static long long unsigned int lastGOut=0;
	for(int i=minCw;i<maxCw;++i){
		if (PRDSTDMODEL.allDist[i].mean==-1){
			if (prdCount[i] != 0) {
				logMessage(stderr,__FUNCTION__,__LINE__,
						"no mean for %d count %d\n",
					i-MAXCW,prdCount[i]);
				++*errorCount;
			}
		} else {
			if (prdCount[i] != 0){
				int d = abs(curState->allRatio[i]-
					PRDSTDMODEL.allDist[i].mean);
				if (d > PRDSTDMODEL.allDist[i].max){
					logAnomaly(now,"ALLMAX",name,i-MAXCW,0,
						PRDSTDMODEL.allDist[i].max,
						PRDSTDMODEL.allDist[i].mean,
						PRDSTDMODEL.allDist[i].stddev,
						curState->allRatio[i],
						d);
					++deviantCount;
					++*alltwo;
					++*allmax;
				}
				else if (d > 2*PRDSTDMODEL.allDist[i].stddev){
					logAnomaly(now,"ALL2STD",name,i-MAXCW,0,
						PRDSTDMODEL.allDist[i].max,
						PRDSTDMODEL.allDist[i].mean,
						PRDSTDMODEL.allDist[i].stddev,
						curState->allRatio[i],
						d);
					++deviantCount;
					++*alltwo;
				}
			}
		}
		for(int j=minCw;j<maxCw;++j){
			if (i == j){continue;}
			if (abs(coreCoefficient[i][j])<CORETHRESH){continue;}
			if (prdCount[i] == 0){
				int zProb=checkZeroProb(i,j,prdCount[j]);
				if (zProb<ZPROBTHRESH){
					++deviantCount;
					++*zCount;
					logZeroAnomaly(now,name,i-MAXCW,j-MAXCW,
						zProb,prdCount[j],ZPROBTHRESH);
				}
				continue;
			}
			if (prdCount[j] == 0){
				int zProb=checkZeroProb(j,i,prdCount[i]);
				if (zProb < ZPROBTHRESH){
					++deviantCount;
					++*zCount;
					logZeroAnomaly(now,name,j-MAXCW,i-MAXCW,
						zProb,prdCount[i],ZPROBTHRESH);
				}
				continue;
			}
			if ((PRDSTDMODEL.dist[i][j].mean==0) && (curState->ratio[i][j]!=0)){
				logMessage(stderr,__FUNCTION__,__LINE__,"%d/%d %d %d %d\n",
				i-MAXCW,j-MAXCW,
				PRDSTDMODEL.dist[i][j].mean,
				curState->ratio[i][j],
				coreCoefficient[i][j]);
				++*errorCount;
				continue;
			}
			if (PRDSTDMODEL.dist[i][j].mean==-1){
				logMessage(stderr,__FUNCTION__,__LINE__,"%d/%d %d %d MEAN -1\n",
				i-MAXCW,j-MAXCW,
				PRDSTDMODEL.dist[i][j].mean,
				curState->ratio[i][j]);
				++*errorCount;
				continue;
			}
			int d = abs(curState->ratio[i][j]-
				PRDSTDMODEL.dist[i][j].mean);
			if (d > PRDSTDMODEL.dist[i][j].max){
				logAnomaly(now,"MAX", name,i-MAXCW,j-MAXCW,
					PRDSTDMODEL.dist[i][j].max,
					PRDSTDMODEL.dist[i][j].mean,
					PRDSTDMODEL.dist[i][j].stddev,
					curState->ratio[i][j],
					d);
				++deviantCount;
				++*max;
				continue;
			}
			if (d > 2*PRDSTDMODEL.dist[i][j].stddev){
				logAnomaly(now,"2STD", name,i-MAXCW,j-MAXCW,
					PRDSTDMODEL.dist[i][j].max,
					PRDSTDMODEL.dist[i][j].mean,
					PRDSTDMODEL.dist[i][j].stddev,
					curState->ratio[i][j],
					d);
				++deviantCount;
				++*two;
				continue;
			}
		}
	}
	fprintf(ssvFile,"%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu %s\n",
		now,
		*allmax,*alltwo,*max,*two,deviantCount,*zCount,*errorCount,name);
	fflush(ssvFile);
	fprintf(stdout, "%10lu %10llu %10llu\r",
		now=time(0),OBSIN,
		totAnom+=(*allmax+*alltwo+*max+*two+deviantCount+*zCount+*errorCount));
	if ((now-lastGOut)>=1){
		fprintf(gFile,"%lu\n",
			(*allmax+*alltwo+*max+*two+deviantCount+*zCount+*errorCount));
		fflush(gFile);
		lastGOut=now;
	}
	return(deviantCount);
}
int initModels(){
	char *e=initCwMap();
	if (e!=0){
		fprintf(stderr,"%s.%d initCwMap(): %s\n",__FUNCTION__,__LINE__,e);
		return(-1);
	}
	setMinMaxCw(&minCw,&maxCw);
	initFlagToCW();
	FILE *modelFile=fopen(STOREDSTDMODELFILE,"re");
	logMessage(stderr,__FUNCTION__,__LINE__,"model file is %s",STOREDSTDMODELFILE);
	if (modelFile==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
			"fopen(%s) failed with %s\n",
			STOREDSTDMODELFILE,strerror(errno));
		return(-1);
	}
	bzero(&PRDSTDMODEL,sizeof(prdStdModel));
	if (fread(&PRDSTDMODEL,sizeof(prdStdModel),1,modelFile) != 1){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"fread() failed %s\n",strerror(errno));
		//return(-1);
	}
	fclose(modelFile);
	if ((modelFile=fopen(STOREDCORCOFFILE,"re"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"fopen(%s) failed with %s\n",
				STOREDCORCOFFILE,strerror(errno));
		for(int i=0;i<MAXCW*2;++i){
			for(int j=0;j<MAXCW*2;++j){coreCoefficient[i][j]=1;}}
		return(-1);
	}
	bzero(coreCoefficient,MAXCW*2*MAXCW*2*sizeof(int));
	if (fread(coreCoefficient,MAXCW*2*MAXCW*2*sizeof(int),1,modelFile)!= 1){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"fread() failed %s\n", strerror(errno));
		//return(-1);
	}
	fclose(modelFile);
	if ((modelFile=fopen(STOREDZPROBFILE,"re"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"fopen failed with: %s\n",strerror(errno));
		return(-1);
	}
	bzero(PZ,MAXCW*2*sizeof(zProb));
	if (fread(PZ,MAXCW*2*sizeof(zProb),1,modelFile) !=1){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"fread failed with: %s\n",strerror(errno));
		//return(-1);
	}
	fclose(modelFile);
	return(0);
}
