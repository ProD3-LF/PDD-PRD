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
#include "../common/pddObs.h"
#include "../common/logMessage.h"
#include "defs.h"
#include "pdd_seq_detector.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <malloc.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#define MAXCW 128
static long long unsigned overFlowTime=0;
extern time_t startTime;
static pthread_mutex_t CS;
void enterCS(){
	pthread_mutex_lock(&CS);
}
void exitCS(){
	pthread_mutex_unlock(&CS);
}
int QFront = 0;
int QRear = -1;
#define QSize 100000
size_t QItemCount = 0;
pddObs *QIntArray[QSize];
pddObs *QPeek() {
   return QIntArray[QFront];
}
bool QIsEmpty() {
	enterCS();
   	bool t = (QItemCount == 0);
	exitCS();
	return(t);
}
bool QIsFull() {
	enterCS();
   	bool t = (QItemCount == QSize);
	exitCS();
	return(t);
}
size_t QItemsInBuffer() {
	enterCS();
   	size_t t=QItemCount;
	exitCS();
	return(t);
}
pddObs *QRemoveData() {
	enterCS();
   	if (QItemCount == 0){
		exitCS();
		return(0);
	}
	pddObs *data = QIntArray[QFront++];
	if(QFront == QSize) {
		QFront = 0;
	}
	QItemCount--;
	exitCS();
	return data;
}
#define EMPTYQWAIT 100
int DONE=0;
void *processQ(void *a){
	while (1==1){
		pddObs *d=QRemoveData();
		if (d!=0){
			processTcp(d);
			free(d);
			continue;
		}
		if (DONE==1){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"processQ exiting");
		       	pthread_exit(0);
			return(0);
		}
		usleep(EMPTYQWAIT);
	}
}
void toQ(pddObs *data) {
	enterCS();
   	if (QItemCount == QSize){
		if (overFlowTime==0){
		       	overFlowTime=data->timeStamp;
		}
		exitCS();
		free(data);
		(*SENDOVERFLOWALERT)();
		return;
	}
	if(QRear == QSize-1) {
		QRear = -1;
	}
	QIntArray[++QRear] = data;
	QItemCount++;
	overFlowTime=0;
	exitCS();
}
size_t obsRecv=0;
int readFromFifo(int *fd){
	ssize_t toRead=sizeof(pddObs),haveRead=0;
	pddObs *d=(pddObs *)malloc(sizeof(pddObs));
	if (d==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"malloc failed");
		return(-1);
	}
	unsigned char *p=(unsigned char *)d;
	while (toRead > 0){
		ssize_t n=read(*fd,&p[haveRead],toRead);
		if (n==-1){
			if (errno == EAGAIN){continue;}
			logMessage(stderr,__FUNCTION__,__LINE__,
				"read fifo %d failed with %s",
				*fd,strerror(errno));
			return(-1);
		}
		if (n==0){
			logMessage(stderr,__FUNCTION__,__LINE__,
				"writer closed fifo %d",*fd);
			close(*fd);
			*fd=0;
			free(d);
			return(0);
		}
		toRead -=n;
		haveRead += n;
	}
	++obsRecv;
	toQ(d);
	return(0);
}
void (*INITALERT)();
void (*FLUSHALERT)();
void (*CLOSEALERT)();
void (*SENDOVERFLOWALERT)();
void (*SENDALERT)(uint32_t serverIP,uint32_t serverPort,uint32_t clientIP,
		uint16_t clientPort, enum SCalert type,int v[],int n);
#define SELECTTIMEOUT 10
#define FIFOERRCNT 100
#define FIFOWAITPERIOD 1000000
void handleSig(int s){
	if (s == SIGTERM)  {
		logMessage(stderr,__FUNCTION__,__LINE__,"got %d setting done\n",s);
		DONE=1;
	}
	if (s == SIGINT)  {
		logMessage(stderr,__FUNCTION__,__LINE__,"got %d exiting\n",s);
		exit(0);
	}
	if (s != SIGCHLD){
	       	logMessage(stderr,__FUNCTION__,__LINE__,"got %d\n",s);
	}
	       	logMessage(stderr,__FUNCTION__,__LINE__,"TFB got %d\n",s);
}
void pddMain(callBacks *c) {
	int fd[NFIFOS],n=0;
	struct timeval t={SELECTTIMEOUT,0};
	struct timeval *tp=&t;
	startTime=time(0);
	INITALERT=c->initAlert;
	FLUSHALERT=c->flushAlert;
	CLOSEALERT=c->closeAlert;
	SENDALERT=c->sendAlert;
	SENDOVERFLOWALERT=c->sendOverflowAlert;
	pddInit();
	pthread_mutex_init(&CS,NULL);
	pthread_t th=0;
	pthread_create(&th,0,processQ,0);
	fprintf(stdout,
		"\e[4m\e[1m             PDD DETECTOR             \e[m\n");
	fprintf(stdout, "%10s %10s %10s\n","TIME","OBSIN","ANOMALIES");
	for(int i=0;i<SIGSYS;++i){
		if ((i!=SIGSEGV) || (i!=SIGCONT)) signal(i,handleSig);
	}
	for(size_t i=0;i<NFIFOS;++i){
		char fifoName[PATH_MAX];
		snprintf(fifoName,sizeof(fifoName),"/tmp/PDDFifo%ld",i);
		while (access(fifoName,R_OK)==-1){
			if ((n++ % FIFOERRCNT) == 0){
				logMessage(stderr,__FUNCTION__,__LINE__,
						"waiting for %s",fifoName);
			}
			usleep(FIFOWAITPERIOD);
		}
		while ((fd[i]=open(fifoName,O_RDONLY|O_NONBLOCK|O_CLOEXEC))==-1){
			logMessage(stderr,__FUNCTION__,__LINE__,
				"open(%s) failed with %s",
					fifoName,strerror(errno));
			usleep(FIFOWAITPERIOD);
		}
	}
	while (1) {
		int closing=0;
		int openFds=0,nfds=0,s=0;
		fd_set rfds;
		FD_ZERO(&rfds);
		for(size_t i=0;i<NFIFOS;++i){
			if (fd[i] != 0){
				++openFds;
				FD_SET(fd[i],&rfds);
				if (nfds < (fd[i]+1)){nfds=fd[i]+1;}
				if (nfds > FD_SETSIZE){
					logMessage(stderr,__FUNCTION__,__LINE__,
							"FD too big");
					exit(-1);
				}
			} else {closing=1;}
		}
		if (openFds == 0){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"all fifos closed\n");
			goto shutdown;
		}
		if (DONE==1) goto shutdown;
		if ((s=select(nfds,&rfds,0,0,tp))==-1){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"select: %s",strerror(errno));
			continue;
		}
		if (s==0){
			if (closing == 1){break;}
			tp->tv_sec=SELECTTIMEOUT;
			continue;
		}
		tp->tv_sec=SELECTTIMEOUT;
		for (size_t i=0;i<NFIFOS;++i){
			if (FD_ISSET(fd[i],&rfds)){
				if (readFromFifo(&fd[i])==0){
					flushThreadObsFd();
					continue;
				}
				logMessage(stderr,__FUNCTION__, __LINE__,
					"readFromFifo failed, exiting");
				goto shutdown;
			}
		}
		(*FLUSHALERT)();
	}
shutdown:
	while(QItemsInBuffer()!=0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"waiting for queue to empty");
		usleep(FIFOWAITPERIOD);
	}
	pthread_cancel(th);
	DONE=1;
	sleep(1);
	flushThreadObsFd();
	logMessage(stderr,__FUNCTION__,__LINE__,"obsRecv=%lu",obsRecv);
	(*FLUSHALERT)();
	(*CLOSEALERT)();
	fflush(stdout);
	fflush(stderr);
	freeModels();
	freeServerData();
}
