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
#include "../common/prdObs.h"
#include "../common/util.h"
#include "defs.h"
#include "prdFileDefs.h"
#include "prdModel.h"
#include "prdStdModel.h"
#include "zProb.h"
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
char hostName[HOST_NAME_MAX];
time_t startTime=0;
time_t obsStartTime=0;
FILE *ssvFile=0;
FILE *gFile=0;
#define VFMT " %80[^{},\n\t]"
long long unsigned int OBSIN=0;
//#define NSDINCR 50000
#define NSDINCR 500
serverData SERVD[NSERVD];
#define SDCLEANTIME 30
void resetCounts(serverData *c){
	for(size_t i=0;i<MAXCW*2;++i){c->prdCount[i]=0;}
	c->prdTotal=0;
}
void initSERVD(){
	for (size_t i=0;i<NSERVD;++i){
		resetCounts(&SERVD[i]);
		SERVD[i].lastCheck = 0;
		SERVD[i].o.obsX=0;
	}
}

#define MINPRECISION .01
#define TOPERCENT 100
double precision(double d){
	if (d > MINPRECISION){return(d);}
	return(MINPRECISION);
}
void updateCur(serverData *c){
	for(int i=0;i<MAXCW;++i){for(int j=0;j<MAXCW;++j){c->curState.ratio[i][j]=0;}}
	for(int i=minCw;i<maxCw;++i){
		double d = precision((double)c->prdCount[i]/(double)c->prdTotal);
		d*=TOPERCENT;
		d=round(d);
		c->curState.allRatio[i] = (int)d;
		for(int j=minCw;j<maxCw;++j){
			if (i==j){
				c->curState.ratio[i][j]=1;
				continue;
			}
			if ((c->prdCount[i]!=0) && (c->prdCount[j]!=0)){
				d = precision((double)c->prdCount[i]/(double)c->prdCount[j]);
				d*=TOPERCENT;
				d=round(d);
				c->curState.ratio[i][j] = (int)d;
			} 
			else{
				c->curState.ratio[i][j]=0;
			}
		}
	}
}
void checkPRD(serverData *c,long long unsigned int obsTime){
	uint64_t zCount=0,errorCount=0,max=0,two=0,allmax=0,alltwo=0;
	updateCur(c);
	if (comparCurToModel(c->name,c->prdCount,&c->curState,obsTime,&zCount,
				&errorCount,&max,&two,&allmax,&alltwo)==0){
		c->CONSEQALLMAX=c->CONALLSEQ2=c->CONZERO=c->CONSEQMAX=
			c->CONSEQ2=0;
		return;
	}
	if (allmax != 0){++c->CONSEQALLMAX;}else{c->CONSEQALLMAX=0;}
	if (alltwo != 0){++c->CONALLSEQ2;}else{c->CONALLSEQ2=0;}
	if (max != 0){++c->CONSEQMAX;}else{c->CONSEQMAX=0;}
	if (two != 0){++c->CONSEQ2;}else{c->CONSEQ2=0;}
	if (zCount !=0){++c->CONZERO;}else{c->CONZERO=0;}
	if (c->CONZERO > 1){
		(*SENDALERT)(c->server,ntohs(c->port),"PRDZERO"); }
	if (c->CONSEQALLMAX > 2){
		(*SENDALERT)(c->server,ntohs(c->port),"PRDALLMAX"); }
	if (c->CONALLSEQ2 > 1){
		(*SENDALERT)(c->server,ntohs(c->port),"PRDALLSD2"); }
	if (c->CONSEQMAX > 2){
	 	(*SENDALERT)(c->server,ntohs(c->port),"PRDMAX");
		return;
	}
	if (c->CONSEQ2 > 2) {
	 	(*SENDALERT)(c->server,ntohs(c->port),"PRDSD2"); }
}
void outputPRDCounts(serverData *c,long long unsigned int obsTime){
	char obsFileName[PATH_MAX];
	static bool init=false;
	char obsFilePath[PATH_MAX];
	if (init==false){
		init=true;
		snprintf(obsFilePath,PATH_MAX,OBSDIR,hostName,startTime);
		if (mkdirEach(obsFilePath)!=0){
			logMessage(stderr,__FUNCTION__,__LINE__, "cannot make obsevation directory %s\n",
				obsFilePath);
			return;
		}
	}
	snprintf(obsFileName,PATH_MAX,PRDOBSFILE,hostName,startTime,obsTime);
	FILE *obsFile=fopen(obsFileName,"w+e");
	if (obsFile==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"fopen(%s): %s\n",
			obsFileName,strerror(errno));
		return;
	}
	for (int i=0;i<MAXCW*2;++i){
		if (c->prdCount[i] != 0){
			fprintf(obsFile,"%d %d\n",i-MAXCW,c->prdCount[i]);
		}
	}
	fclose(obsFile);
	c->lastCheck=obsTime;
}
void initCounts(){
	for(size_t i=0;i<NSERVD;++i){
		resetCounts(&SERVD[i]);
	}
}
void countCw(serverData *c,int cw,unsigned int n){
	int acw=0;
	if (n==0){return;}
	acw = cw+MAXCW;
	c->prdCount[acw]+=n;
	c->prdTotal+=n;
	if (acw<c->curState.low){ c->curState.low=acw; }
	if (acw>c->curState.high-1){ c->curState.high=acw+1; }
};
void dumpPRDCnt(serverData *c,long long unsigned int start,long long unsigned int stop){
	logMessage(stderr,__FUNCTION__,__LINE__,"--------------------SLIDINGWINDOW %llu  %llu--------------\n",start-PRDOBSTIME,stop);
	for(int i=0;i<MAXCW*2;++i){
		if (c->prdCount[i]!=0){
		       char cws[SHORTSTRING];
		       convertCw(i-MAXCW,cws,SHORTSTRING);
	       	       logMessage(stderr,__FUNCTION__,__LINE__,"CWDUMP %s %d\n",cws,c->prdCount[i]);
		}
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"----------%lu----------\n",c->prdTotal);
}
#define WARMUPPERIOD 30
void addObs(serverData *s,nodeCwCnt *p){
	if (s->o.obsX == NINT){
		logMessage(stderr,__FUNCTION__,__LINE__,"CANNOT ADD OBS\n");
		return;
	}
	s->o.obs[s->o.obsX++]=*p;
//	logMessage(stderr,__FUNCTION__,__LINE__,"ADDED OBS\n");
}
void delOldObs(serverData *s,long long unsigned int t){
	size_t i=0,toRemove=0;
	for(;i<s->o.obsX;++i){
		if ((t-s->o.obs[i].lastOutputTime)<=PRDCHECKTIME){break;}
		++toRemove;
	}
	if (toRemove == 0){ return; }
	for(size_t k=0;i<s->o.obsX;++i,++k){
		s->o.obs[k] = s->o.obs[i];
	}
	s->o.obsX-=toRemove;
}
static serverData sd;
size_t NSD=0;
#define MAXSD 1
void initSD(uint32_t serverIP,uint16_t serverPort){
	if (NSD >= MAXSD){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"can only have one server");
		exit(-1);
	}
	sd.CONZERO=sd.CONSEQMAX=sd.CONSEQ2=sd.CONSEQALLMAX=sd.CONALLSEQ2=0;
	sd.lastCheck = 0;
	sd.o.obsX=0;
	sd.server=serverIP;
	sd.port=ntohs(serverPort);
	sprintf(sd.name,"%08x.%u",serverIP,serverPort);
	++NSD;
}
serverData *getSd(uint32_t serverIP,uint16_t serverPort){
	if ((sd.server==serverIP) && (sd.port==serverPort)){
		return(&sd);
	}
	logMessage(stderr,__FUNCTION__,__LINE__,
			"%08x.%d is not server %08x.%d",
			serverIP,serverPort,sd.server,sd.port);
	return(0);
}

void updateCounts(serverData *sd){
	resetCounts(sd);
	for (size_t in=0;in<NINT;++in){
		for (size_t i=0;i<MAXCW;++i){
			countCw(sd,flagToCW[i]*Received,
					sd->o.obs[in].prdCountIn[i]);
			countCw(sd,flagToCW[i]*Sent,
					sd->o.obs[in].prdCountOut[i]);
		}
	}
}
void processTcp(nodeCwCnt *p){
	//TO DO HANDLE MULTIPLE SERVERS
	++OBSIN;
	serverData *sd=getSd(p->serverIP,p->serverPort);
	if (sd==0) {return;}
	if (sd->o.obsX < NINT) {
		addObs(sd,p);
		return;
	}
	updateCounts(sd);
	if (adRecord==true){outputPRDCounts(sd,msecTime(0));}
	if (adDetect==true){checkPRD(sd,msecTime(0));}
	sd->lastCheck=time(0);
	delOldObs(sd,p->lastOutputTime+PRDOBSTIME);
	addObs(sd,p);
}
