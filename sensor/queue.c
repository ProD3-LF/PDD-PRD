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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <netinet/in.h>
#include "bucket.h"
#include "defs.h"
#include "../common/prdObs.h"
#include "../common/logMessage.h"
static bucket B[NFIFOS];
int pddFifoFd[NFIFOS];
int prdFifoFd=0;
void initBucket(){
	for (size_t i=0;i<NFIFOS;++i){
		bucket *b=&B[i];
		b->p.pddObsX=0;
		b->p.timeStamp=0;
		b->p.sample=b->SAMPLE=1;
		b->outTime=0;
		b->inTime=0;
		b->inStartTime=0;
		b->inTime=0;
		b->lastObs=0;
		b->lastSampleChange=0;
	}
}		
double pddFifoFailRate[NFIFOS];
double prdFifoFailRate=0;
void toPrdFifo(nodeCwCnt *item){
	int n;
	static size_t noWriteError=0,againError=0;
	size_t toWrite,haveWritten;
	unsigned char *p=(unsigned char *)item;
	toWrite=sizeof(*item);
	haveWritten=0;
	if (prdFifoFd == 0){
		char fifoName[80];
		prdFifoFailRate=0;
		sprintf(fifoName,"/tmp/PRDFifo");
		while ((prdFifoFd=open(fifoName, O_WRONLY|O_NONBLOCK))==-1){
			logMessage(stderr,__FUNCTION__,__LINE__,
				"open %s failed %s",
				fifoName,strerror(errno));
			prdFifoFd=0;
			return;
		}
	}
	while (toWrite > 0){
		if ((n=write(prdFifoFd,&p[haveWritten],toWrite))==-1){
			if (errno == EAGAIN) ++againError;
			else {
				logMessage(stderr,__FUNCTION__,__LINE__,
				"TFB write %s %ld %ld",
				strerror(errno),toWrite,haveWritten);
			}
			continue;
		}
		noWriteError++;
		toWrite -= n;
		haveWritten += n;
	}
	prdFifoFailRate=(double)againError/(double)noWriteError;
	if (prdFifoFailRate>1) logMessage(stderr,__FUNCTION__,__LINE__,
			"TFB prdFifoFailRate=%g",
			prdFifoFailRate);
}
size_t obsSent=0;
void toPddFifo(pddObs *item,size_t N){
	int n;
	size_t toWrite,haveWritten;
	unsigned char *p=(unsigned char *)item;
	toWrite=sizeof(*item);
	haveWritten=0;
	if (pddFifoFd[N] == -1){
		char fifoName[80];
		pddFifoFailRate[N]=0;
		sprintf(fifoName,"/tmp/PDDFifo%ld",N);
		while ((pddFifoFd[N]=open(fifoName, O_WRONLY|O_NONBLOCK))==-1){
			logMessage(stderr,__FUNCTION__,__LINE__,
				"open %s failed %s",
				fifoName,strerror(errno));
			pddFifoFd[N]=0;
			usleep(1000);
		}
		logMessage(stderr,__FUNCTION__,__LINE__,
				"opened %s %lu %d",
				fifoName,N,pddFifoFd[N]);
	}
	//logMessage(stderr,__FUNCTION__,__LINE__,"TFB %lu %lu",toWrite,N);
	while (toWrite > 0){
		if ((n=write(pddFifoFd[N],&p[haveWritten],toWrite))==-1){
			if (errno != EAGAIN){
				logMessage(stderr,__FUNCTION__,__LINE__,
				"TFB write %s %ld %ld %ld",
				strerror(errno),toWrite,haveWritten,N);
			}
			continue;
		}
		toWrite -= n;
		haveWritten += n;
	}
	++obsSent;
}
void prdDetectorInsert(nodeCwCnt *data){
	toPrdFifo(data);
}
size_t getFifo(uint16_t port){
	return(port % NFIFOS);
}
extern nodeCwCnt *getNC(in_addr_t IP,uint32_t Port);
void handlePRDObs(double obsTime,in_addr_t serverIP,uint32_t serverPort,uint8_t flag,int8_t direction){
	long long unsigned int now = obsTime*1000;
	nodeCwCnt *nc=getNC(serverIP,serverPort);
	if (nc == 0){
		logMessage(stderr,__FUNCTION__,__LINE__,"no NC\n");
		return;
	}
	if (nc->lastOutputTime == 0) nc->lastOutputTime=now;
	if (nc->lastOutputTime > now){
		logMessage(stderr,__FUNCTION__,__LINE__,"THIS CANNOT BE: %llu %llu",nc->lastOutputTime,now);
	}
	long long unsigned int d = now - nc->lastOutputTime;
	if (d<PRDOBSTIME){
		if (direction == Sent){
			++nc->prdCountOut[flag];
		} else {
			++nc->prdCountIn[flag];
		}
		++nc->prdTotal;
		if (nc->lastOutputTime > now){
			logMessage(stderr,__FUNCTION__,__LINE__,"THIS CANNOT BE: %llu %llu",nc->lastOutputTime,now);
		}
		return;
	}
	if (d==PRDOBSTIME){
		if (direction == Sent){
			++nc->prdCountOut[flag];
		} else {
			++nc->prdCountIn[flag];
		}
		++nc->prdTotal;
		if (nc->lastOutputTime > now){
			logMessage(stderr,__FUNCTION__,__LINE__,"THIS CANNOT BE: %llu %llu",nc->lastOutputTime,now);
		}
		nc->lastOutputTime=now;
		prdDetectorInsert(nc);
		return;
	}
	if (d > 2*PRDOBSTIME){
		logMessage(stderr,__FUNCTION__,__LINE__,"coming out of quiet time %llu %llu",
					nc->lastOutputTime,now);
		nc->lastOutputTime=now;
		resetCwCnts(nc);
		if (direction == Sent){
			++nc->prdCountOut[flag];
		} else {
			++nc->prdCountIn[flag];
		}
		++nc->prdTotal;
		return;
	}
	nc->lastOutputTime=nc->lastOutputTime+PRDOBSTIME;
	prdDetectorInsert(nc);
	resetCwCnts(nc);
	if (direction == Sent){
		++nc->prdCountOut[flag];
	} else {
		++nc->prdCountIn[flag];
	}
	++nc->prdTotal;
	if (nc->lastOutputTime > now){
		logMessage(stderr,__FUNCTION__,__LINE__,"THIS CANNOT BE: %llu %llu",nc->lastOutputTime,now);
	}
	return;
}
void handlePDDObs(in_addr_t serverIP,in_addr_t clientIP,uint32_t serverPort, uint32_t clientPort,uint8_t flag,int8_t direction){
	unsigned long long int deltaTime;
	size_t FIFON;
	pddObs *t;
	bucket *b;
	//uint16_t pi=ntohs(clientPort);
	uint16_t pi=clientPort;
	FIFON=getFifo(pi);
	b=&B[FIFON];
	t=&b->p;
	t->pd[t->pddObsX].clientIP=clientIP;
	t->pd[t->pddObsX].clientPort=pi;
	t->pd[t->pddObsX].serverIP=serverIP;
	t->pd[t->pddObsX].serverPort=serverPort;
	t->pd[t->pddObsX].cw=flag;
	t->pd[t->pddObsX].direction=direction;
	++t->pddObsX;
	if (t->timeStamp==0) t->timeStamp=msecTime();
	deltaTime=msecTime()-t->timeStamp;
	if ((t->pddObsX<MAXOBS) && (deltaTime < MAXOBSTIME)){
		return;
	}
	t->timeStamp=msecTime();
	t->bucket=FIFON;
	toPddFifo(t,t->bucket);
	t->pddObsX=0;
}
void flushBuckets(){
	pddObs *t;
	bucket *b;
	for(size_t i=0;i<NFIFOS;++i){
		b=&B[i];
		t=&b->p;
		if (t->pddObsX != 0){
			t->timeStamp=msecTime();
			t->bucket=i;
			toPddFifo(t,t->bucket);
			t->pddObsX=0;
		}
	}
}
void closeFifos(){
	char t='\0';
	for(size_t i=0;i<NFIFOS;++i){
		if (pddFifoFd[i]!=-1){
			if (write(pddFifoFd[i],&t,0)!=0){
				logMessage(stderr,__FUNCTION__,__LINE__,
						"%lu write(%d) %s",
						i,pddFifoFd[i],strerror(errno));
			}
			usleep(1000);
		       	close(pddFifoFd[i]);
			logMessage(stderr,__FUNCTION__,__LINE__,"closed(%d) %lu",pddFifoFd[i],i);
		} else {
			logMessage(stderr,__FUNCTION__,__LINE__,"NOT closed(%d) %lu",pddFifoFd[i],i);
		}
		usleep(1000);
	}
}
