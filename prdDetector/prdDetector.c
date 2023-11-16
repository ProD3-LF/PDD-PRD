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
#include "../common/logMessage.h"
#include "../common/prdObs.h"
#include "defs.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
void handleSig(int s){
	if ((s == SIGTERM) || (s == SIGINT))  {
		logMessage(stderr,__FUNCTION__,__LINE__,"got %d exiting\n",s);
		exit(0);
	}
	if (s != SIGCHLD){
	       	logMessage(stderr,__FUNCTION__,__LINE__,"got %d\n",s);
	}
}
void (*INITALERT)();
void (*FLUSHALERT)();
void (*CLOSEALERT)();
void (*SENDOVERFLOWALERT)();
void (*SENDALERT)(uint32_t serverIP,uint16_t serverPort,char *violation);
int readFromFifo(int fd,nodeCwCnt *d){
	ssize_t toRead=sizeof(*d),haveRead=0;
	unsigned char *p=(unsigned char *)d;
	while (toRead > 0){
		ssize_t n=read(fd,&p[haveRead],toRead);
		if (n==-1){
			if (errno == EAGAIN){continue;}
			logMessage(stderr,__FUNCTION__,__LINE__,
				"read fifo %d failed with %s",
				fd,strerror(errno));
			return(-1);
		}
		if (n==0){
			logMessage(stderr,__FUNCTION__,__LINE__,
				"writer closed fifo %d",fd);
			close(fd);
			return(-1);
		}
		toRead -=n;
		haveRead += n;
	}
	return(0);
}

#define FIFOWAITPERIOD 1000
void prdMain(callBacks *c) {
	int fd=0;
	long long unsigned int firstOutputTime=0;
	startTime=time(0);
	nodeCwCnt d;
	INITALERT=c->initAlert;
	FLUSHALERT=c->flushAlert;
	CLOSEALERT=c->closeAlert;
	SENDALERT=c->sendAlert;
	fprintf(stdout,"\e[4m\e[1m	   PRD DETECTOR	      \e[m\n");
	fprintf(stdout, "%10s %10s %10s\n","TIME","OBSIN","ANOMALIES");
	for(int i=0;i<SIGSYS;++i){
		if (i!=SIGSEGV) signal(i,handleSig);
	}
	prdInit();
	while (access("/tmp/PRDFifo",R_OK)==-1){
	    usleep(FIFOWAITPERIOD);
	}
	while ((fd = open("/tmp/PRDFifo", O_RDONLY|O_NONBLOCK|O_CLOEXEC))==-1){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"FIFO open failed %s",strerror(errno));
		while (access("/tmp/PRDFifo",R_OK)==-1){
	    		logMessage(stderr,__FUNCTION__,__LINE__,
					"waiting for fifo");
	    		usleep(FIFOWAITPERIOD);
		}
	}
	while (true){
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(fd,&rfds);
		int nfds=fd+1;
		int s=select(nfds,&rfds,0,0,0);
		if ((s==1) && (FD_ISSET(fd,&rfds))){
			if (readFromFifo(fd,&d)!=0){
				logMessage(stderr,__FUNCTION__,__LINE__,
					"input fifo closed, stopping");
				break;
			}
			processTcp(&d);
			if (firstOutputTime == 0){
				firstOutputTime=d.lastOutputTime;}
			fflush(stdout);
			continue;
		}
		if (s<=0){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"select: %d %s\n",s,strerror(errno));
			continue;
		}
		if (s > 1){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"select: too many fds %d\n",s);
			continue;
		}
		if (!FD_ISSET(fd,&rfds)){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"select fired on unknown fd\n");
				
			continue;
		}
	}
	(*FLUSHALERT)();
	(*CLOSEALERT)();
	close(fd);
}
