/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2012-2023 Applied Communication Sciences
 * (now Peraton Labs Inc.)
 *
 * This software was developed in work supported by the following U.S.
 * Government contracts:
 *
 * TBD-XXXXX-TBD
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
#include "../common/pddObs.h"
#include "../common/util.h"
#include "defs.h"
#include "pddFileDefs.h"
#include "pddModel.h"
#include "pdd_seq_detector.h"
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <pthread.h>
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
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
char hostName[HOST_NAME_MAX];
time_t startTime=0;
time_t obsStartTime=0;
FILE *ssvFile=0;
FILE *gFile=0;
int coreCoefficient[MAXCW*2][MAXCW*2];
//#define NSDINCR 50000
#define NSDINCR 2
struct {
	uint32_t client;
	uint16_t clientPort;
	time_t lastPacket;
	pddDetector d;
} typedef sessionData;
struct {
	long long unsigned int lastCheck;
	uint32_t server;
	uint16_t port;
	sessionData **SD[MAXPORTS];
	size_t NSD[MAXPORTS];
	size_t DSX[MAXPORTS];
} typedef serverData;
serverData SERVD[NSERVD];
size_t SERVDX=0;
void freeSessionData(serverData *sd){
	for(size_t i=0;i<MAXPORTS;++i){
		if (sd->SD[i] != 0){
			for(size_t k=0;k<sd->DSX[i];++k){
				if (sd->SD[i][k] != 0){
		       			free(sd->SD[i][k]);
				}
			}
			free(sd->SD[i]);
		}
	}
}
void freeServerData(){
	for(size_t i=0;i<SERVDX;++i){
		freeSessionData(&SERVD[i]);
	}
}
unsigned int SAMPLERATE[NFIFOS];
unsigned int lastSample[NFIFOS];
long long unsigned int sampleChange[MAXPORTS];
void adjSample(unsigned int s,size_t bucket,long long unsigned int now){
	if (lastSample[bucket]==-1){
		SAMPLERATE[bucket]=s;
	       	lastSample[bucket]=s;
		return;
	}
	if (s<=lastSample[bucket]){return;}
	logMessage(stderr,__FUNCTION__,__LINE__,"sample rate changed for bucket %ld from %d to %d",
			bucket,lastSample[bucket],s);
	SAMPLERATE[bucket]=s;
	lastSample[bucket]=s;
	for (size_t j=0;j<MAXPORTS;++j){
		if ((j % NFIFOS) != bucket){continue;}
		if ((j % (NFIFOS*s)) == bucket){continue;}
		sampleChange[j]=now;
	}
	for (size_t i=0;i<SERVDX;++i){
		serverData *sd=&SERVD[i];
		for (size_t j=0;j<MAXPORTS;++j){
			if ((j % NFIFOS) != bucket){continue;}
			if ((j % (NFIFOS*s)) == bucket){continue;}
			for(size_t p=0;p<sd->DSX[j];++p){
				if (sd->SD[j][p]==0){
				       logMessage(stderr,__FUNCTION__,__LINE__,"null sessData");
			       	      continue;
				}
				sessionData *t=sd->SD[j][p];
				if (t!=0){t->d.uninterruptedCWCount=0;}
			}
		}
	}
}
void auditSession(sessionData *sd, long long unsigned int now){
	if (sd->d.idle==true){
	       	return;
	}
	if ((now-sd->lastPacket)<TCPIDLETIME) {
		return;
	}
	if ((sd->d.nScs<MAXSCVEC) && (sd->d.nScs!=sd->d.uninterruptedCWCount)){
		logMessage(stderr,__FUNCTION__,__LINE__,"interrupted start sequence");
	}
	sd->d.idle=true;
	if ((sd->d.nScs<MAXSCVEC) && (sd->d.nScs==sd->d.uninterruptedCWCount)){
		(*SENDALERT)(sd->d.serverIP,sd->d.serverPort,sd->d.clientIP,
			sd->d.clientPort,SC_START_SHORT,
			&sd->d.detectVec_[sd->d.detectVecI_-MAXSCVEC],
			sd->d.nScs);
		return;
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"ABANDONED %08x.%d %llu %llu",
		ntohl(sd->client),sd->clientPort,now,sd->lastPacket);
}
void auditServer(serverData *sd, long long unsigned int now){
	for (size_t cp=0;cp<MAXPORTS;++cp){
		for (size_t sp=0;sp<sd->DSX[cp];++sp){
			auditSession(sd->SD[cp][sp],now);
		}
	}
}
void audit(long long unsigned int obsTime,long long unsigned int now){
	static long long unsigned lastAuditTime=0;
	if (lastAuditTime==0){
		lastAuditTime=now;
	}
	if ((now-obsTime)<TCPIDLETIME){return;}
	for (size_t i=0;i<SERVDX;++i){
		auditServer(&SERVD[i],obsTime);
	}
	lastAuditTime=msecTime();
}
#define HIGH 0
#define MED 1
#define LOW 2
void initSERVD(){
	for (size_t i=0;i<NSERVD;++i){
		SERVD[i].lastCheck = 0;
	}
}

#define MAXSERVER 10
uint16_t knownServerPort[MAXSERVER];
in_addr_t knownServerIP[MAXSERVER];
size_t knownServerN=0;
uint16_t ephLow,ephHigh;
bool knownServer(uint16_t p,in_addr_t ip){
	for(size_t i=0;i<knownServerN;++i){
		if ((knownServerPort[i]==p) && (knownServerIP[i]==ip)){
		       	return(true);}
	}
	return false;
}
void addKnownServer(uint16_t p,in_addr_t ip){
	logMessage(stderr,__FUNCTION__,__LINE__,"new server %x.%u at %lu",ip,p,knownServerN);
	knownServerPort[knownServerN]=ntohs(p); //leave in network order for
						//faster inline compare
	knownServerIP[knownServerN++]=ip;
}
void clearOldSessionData(time_t now){
	static time_t lastClean=0;
	if (lastClean==0) {
		lastClean = now;
		return;
	}
	if ((now - lastClean) < TCPIDLETIME){return;}
	for(size_t k=0;k<SERVDX;++k){
		serverData *sd=&SERVD[k];
		for (size_t port=0;port<MAXPORTS;++port){
			for (size_t i=0;i<sd->DSX[port];++i){
				if ((now-sd->SD[port][i]->lastPacket) >
					       	TCPIDLETIME){
					free(sd->SD[port][i]);
					for(size_t j=i;j<sd->DSX[port]-1;++j){
						sd->SD[port][j]=sd->SD[port][j+1];
					}
					sd->DSX[port]--;
					--i;
				}
			}
			if ((sd->DSX[port]!=0)&&(sd->DSX[port]<sd->NSD[port])){
				sessionData **t =(sessionData **)realloc(sd->SD[port],sizeof(sessionData *)*sd->DSX[port]);
				if (t==0){
					logMessage(stderr,__FUNCTION__,__LINE__,"realloc failed");
				} else {
					sd->SD[port]=t;
					sd->NSD[port]=sd->DSX[port];
				}
			}

		}
		lastClean=now;
	}
}

sessionData *insertSessionData(int rc,int m, uint32_t server,
		uint16_t serverPort,uint32_t client,uint16_t clientPort,
		serverData *sd,long long unsigned int obsTime){
	if (sd->DSX[clientPort] >= sd->NSD[clientPort]){
		logMessage(stderr,__FUNCTION__,__LINE__,"DSX > NSD!");
	       	return(0);
	}
	sessionData *t=(sessionData *)malloc(sizeof(sessionData));
	if (t==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"malloc failed");
		exit(-1);
	}
	initPddDetector(&t->d,server,serverPort,client,clientPort,obsTime);
	if (rc < 0){
		for(int i=sd->DSX[clientPort];i>m;--i){
			sd->SD[clientPort][i]=sd->SD[clientPort][i-1];
		}
		sd->SD[clientPort][m]=t;
		sd->DSX[clientPort]++;
		sd->SD[clientPort][m]->client=client;
		sd->SD[clientPort][m]->clientPort=clientPort;
		sd->SD[clientPort][m]->lastPacket = (time_t)obsTime;
		return(sd->SD[clientPort][m]);
	}
	for(int i=sd->DSX[clientPort];i>m+1;--i){
		sd->SD[clientPort][i]=sd->SD[clientPort][i-1];
	}
	sd->SD[clientPort][m+1]=t;
	sd->SD[clientPort][m+1]->client=client;
	sd->SD[clientPort][m+1]->clientPort=clientPort;
	sd->SD[clientPort][m+1]->lastPacket = (time_t)obsTime;
	sd->DSX[clientPort]++;
	return(sd->SD[clientPort][m+1]);
}
/*int compareSessData(sessionData *sd,uint32_t client){
		return(client-sd->client);
}*/
int unsignedComp(unsigned int u1,unsigned int u2){
        if (u1 < u2){return(-1);}
        if (u1 > u2){return(1);}
       return(0);
}
sessionData *findOrAddSessionData(uint32_t server,uint16_t serverPort,
		uint32_t client,uint32_t clientPort,
		serverData *sd,long long unsigned int obsTime){
	long int s=0,m=0,rc=0;
	if (sd->DSX[clientPort] == 0){
		if ((sd->SD[clientPort][0]=
			(sessionData *)malloc(sizeof(sessionData)))==0){
			logMessage(stderr,__FUNCTION__,__LINE__,"malloc failed");
			exit(-1);
		}
		sd->SD[clientPort][0]->client=client;
		sd->SD[clientPort][0]->clientPort=clientPort;
		sd->SD[clientPort][0]->lastPacket = (time_t)obsTime;
		sd->DSX[clientPort]++;
		initPddDetector(&sd->SD[clientPort][0]->d,server,serverPort,
				client,clientPort,obsTime);
		return(sd->SD[clientPort][0]);
	}
	long int e=sd->DSX[clientPort]-1;
	while (s <= e){
		m = (s+e)/2;
		sessionData *t=sd->SD[clientPort][m];
		if (t == 0) {
			logMessage(stderr,__FUNCTION__,__LINE__,"no sess data for %08x.%d at %d of %ld",
				client,clientPort,m,sd->DSX[clientPort]);
		}
		if ((rc=unsignedComp(client,sd->SD[clientPort][m]->client))==0){
			sd->SD[clientPort][m]->lastPacket = (time_t)obsTime;
			return(sd->SD[clientPort][m]);
		}
		if (rc < 0) {
			e = m-1;
		} else {
			s = m+1;
		}
	}
	return(insertSessionData(rc,m,server,serverPort,client,clientPort,sd,
				obsTime));
}
int growSessionData(serverData *sd,uint16_t p){
	sessionData **t=(sessionData **)realloc(sd->SD[p],
			sizeof(sessionData *)*(sd->NSD[p]+NSDINCR));
	if (t==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"realloc failed");
		return(0);
	}
	sd->SD[p]=t;
	for(size_t i=sd->NSD[p];i<sd->NSD[p]+NSDINCR;++i){
		sd->SD[p][i]=(sessionData *)0;
	}
	sd->NSD[p]+=NSDINCR;
	return(1);
}
void setupSDClientPort(serverData *sd,uint16_t clientPort){
	if ((sd->SD[clientPort]=malloc(sizeof(sessionData* )*NSDINCR))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"malloc failed");
		exit(-1);;
	}
	for(size_t j=0;j<NSDINCR;++j){
		sd->SD[clientPort][j]=(sessionData *)0;
	}
	sd->NSD[clientPort]=NSDINCR;
	sd->DSX[clientPort]=0;
}
serverData *getServerData(uint32_t ip,uint16_t port,uint16_t clientPort){
	size_t i=0;
	for(i=0;i<SERVDX;++i){
		if ((SERVD[i].server==ip) && (SERVD[i].port==port)){
			if (SERVD[i].SD[clientPort] == 0){
				setupSDClientPort(&SERVD[i],clientPort);
			}
			return(&SERVD[i]);
		}
	}
	if (i >= NSERVD) {
		logMessage(stderr,__FUNCTION__,__LINE__,"no room");
		return(0);
	}
	SERVD[i].server=ip;
	SERVD[i].port=port;
	for(size_t j=0;j<MAXPORTS;++j){
	       	SERVD[i].SD[j]=0;
	}
	setupSDClientPort(&SERVD[i],clientPort);
	SERVD[i].NSD[clientPort]=NSDINCR;
	SERVD[i].DSX[clientPort]=0;
	++SERVDX;
	return(&SERVD[i]);
}

sessionData *getSessionData(uint32_t client, uint16_t clientPort,
	uint32_t server, uint16_t serverPort,long long unsigned int obsTime) {
	serverData *sd =getServerData(server,serverPort,clientPort);
	if(sd==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"no serverData %x:%x %x:%x",
	   		client,clientPort,server, serverPort);
		return(0);
	}
	if (sd->DSX[clientPort] >= sd->NSD[clientPort]){
		if (sd->DSX[clientPort] >= sd->NSD[clientPort]){
			if (growSessionData(sd,clientPort)==0){
				return(0);
			}
		}
	}
	sessionData *sessd=findOrAddSessionData(server,serverPort,client,clientPort, sd,obsTime);
	if (sessd==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"findOrAddSessionData failed");
		return(0);
	}
	return(sessd);
}
void processObs(pddData *pd,long long unsigned pTime,size_t bucket){
	sessionData *sessd;
	if ((sessd=getSessionData(pd->clientIP,pd->clientPort,
		pd->serverIP,pd->serverPort, pTime))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
			"no sessData %x:%x %x:%x",
	   		pd->clientIP,pd->clientPort,pd->serverIP,
			pd->serverPort);
		return;
	}
	int cw=(flagToCW[pd->cw]*pd->direction);
	if (cw==0){
		logMessage(stderr, __FUNCTION__,__LINE__,
			"IMPOSSIBLE UNKNOWN CW: %s",
			flagsAsString(pd->cw));
		return;
	}
	processCW(&sessd->d,cw,pTime,bucket);
}
void processTcp(pddObs *p){
	static long long unsigned int ptime=0;
	static size_t nObs=0;
	time_t now=msecTime();
	++nObs;
	if ((adRecord==false) && (adDetect==false)){return;}
	adjSample(p->sample,p->bucket,p->timeStamp);
	clearOldSessionData(now);
	for(size_t i=0;i<p->pddObsX;++i){
		processObs(&p->pd[i],p->timeStamp,p->bucket);
	}
	(*FLUSHALERT)(now);
	audit(p->timeStamp,now);
	ptime+=(msecTime()-p->timeStamp);
}
