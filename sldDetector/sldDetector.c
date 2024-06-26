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
#include "../common/decay.h"
#include "../common/logMessage.h"
#include "../common/sldObs.h"
#include "defs.h"
#include "sldAlert.h"
#include "sldAlertTypes.h"
#include "sldDetector.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define TIMELOWMAX 0.000072
#define TIMEHIGHMAX 155.704556
#define TIMELOW 0.815853
#define TIMEHIGH 123.365579
#define COUNTLOWMAX 53
#define COUNTHIGHMAX 2357
#define COUNTLOW 2
#define COUNTHIGH 4074
double TIME0=0,TIME100=0,TIME03=0,TIME97=0;
double COUNT0=0,COUNT100=0,COUNT03=0,COUNT97=0;
void (*INITALERT)();
void (*FLUSHALERT)();
void (*CLOSEALERT)();
void (*SENDALERT)(uint32_t serverIP,uint32_t serverPort,SLDALERTANOM type,
		char *details);

DqDecayData timeAvg,countAvg;;
#define NSDINCR 100
//struct in_addr serverIP;
void initSessionData(sessionData *sd,uint32_t IP,uint16_t port,double pTime){
	sd->clientIP=IP;
	sd->clientPort=port;
	sd->activeCount=0;
	sd->state=0;
	sd->startActive=pTime;
	sd->lastPacketTime=pTime;
	if (sd->startActive ==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"bad ptime\n");
		exit(-1);
	}
	initDecayData(&sd->pAvg);
}
serverData SERVD[NSERVD];
size_t SERVDX=0;
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

void handleSig(int s){
	if ((s == SIGTERM) || (s == SIGINT))  {
		logMessage(stderr,__FUNCTION__,__LINE__,"got %d exiting\n",s);
		exit(0);
	}
	if (s != SIGCHLD){
	       	logMessage(stderr,__FUNCTION__,__LINE__,"got %d\n",s);
	}
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
	for(size_t j=0;j<=MAXPORTS;++j){
	       	SERVD[i].SD[j]=0;
	}
	setupSDClientPort(&SERVD[i],clientPort);
	SERVD[i].NSD[clientPort]=NSDINCR;
	SERVD[i].DSX[clientPort]=0;
	++SERVDX;
	return(&SERVD[i]);
}
sessionData *insertSessionData(long int rc,long int m,uint32_t client,
		uint16_t clientPort, serverData *sd,
		long long unsigned int obsTime){
	if (sd->DSX[clientPort] >= sd->NSD[clientPort]){
		logMessage(stderr,__FUNCTION__,__LINE__,"DSX > NSD!");
	       	return(0);
	}
	sessionData *t=(sessionData *)malloc(sizeof(sessionData));
	if (t==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"malloc failed");
		exit(-1);
	}
	initSessionData(t,client,clientPort,(double)obsTime);
	if (rc < 0){
		for(int i=sd->DSX[clientPort];i>m;--i){
			sd->SD[clientPort][i]=sd->SD[clientPort][i-1];
		}
		sd->SD[clientPort][m]=t;
		sd->DSX[clientPort]++;
		sd->SD[clientPort][m]->clientIP=client;
		sd->SD[clientPort][m]->clientPort=clientPort;
		sd->SD[clientPort][m]->lastPacket = (time_t)obsTime;
		return(sd->SD[clientPort][m]);
	}
	for(int i=sd->DSX[clientPort];i>m+1;--i){
		sd->SD[clientPort][i]=sd->SD[clientPort][i-1];
	}
	sd->SD[clientPort][m+1]=t;
	sd->SD[clientPort][m+1]->clientIP=client;
	sd->SD[clientPort][m+1]->clientPort=clientPort;
	sd->SD[clientPort][m+1]->lastPacket = (time_t)obsTime;
	sd->DSX[clientPort]++;
	return(sd->SD[clientPort][m+1]);
}
int unsignedComp(unsigned int u1,unsigned int u2){
	if (u1 < u2){return(-1);}
	if (u1 > u2){return(1);}
	return(0);
}
sessionData *findOrAddSessionData(uint32_t client,uint32_t clientPort,
		serverData *sd,double obsTime){
	long int s=0;
	long int m=0;
	long int rc=0;
	if (sd->DSX[clientPort] == 0){
		if ((sd->SD[clientPort][0]=
			(sessionData *)malloc(sizeof(sessionData)))==0){
			logMessage(stderr,__FUNCTION__,__LINE__,"malloc failed");
			exit(-1);
		}
		sd->SD[clientPort][0]->clientIP=client;
		sd->SD[clientPort][0]->clientPort=clientPort;
		sd->SD[clientPort][0]->lastPacket = (time_t)obsTime;
		sd->DSX[clientPort]++;
		initSessionData(sd->SD[clientPort][0],client,clientPort,obsTime);
		return(sd->SD[clientPort][0]);
	}
	long int e=(long int)sd->DSX[clientPort]-1;
	while (s <= e){
		m = (s+e)/2;
		sessionData *t=sd->SD[clientPort][m];
		if (t == 0) {
			logMessage(stderr,__FUNCTION__,__LINE__,"no sess data for %08x.%d at %d of %ld",
				client,clientPort,m,sd->DSX[clientPort]);
		}
		if ((rc=unsignedComp(client,sd->SD[clientPort][m]->clientIP))==0){
			sd->SD[clientPort][m]->lastPacket = (time_t)obsTime;
			return(sd->SD[clientPort][m]);
		}
		if (rc < 0) {
			e = m-1;
		} else {
			s = m+1;
		}
	}
	return(insertSessionData(rc,m,client,clientPort,sd,
				(long long unsigned)obsTime));
}
int growSessionData(serverData *sd,uint16_t p){
	sessionData **t=(sessionData **)realloc(sd->SD[p],
			sizeof(sessionData *)*(sd->NSD[p]+NSDINCR));
	if (t==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"realloc failed");
		return(0);
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"growing for port %lu",ntohs(p));
	sd->SD[p]=t;
	for(size_t i=sd->NSD[p];i<sd->NSD[p]+NSDINCR;++i){
		sd->SD[p][i]=(sessionData *)0;
	}
	sd->NSD[p]+=NSDINCR;
	return(1);
}

sessionData *getSessionData(uint32_t client, uint16_t clientPort,
	uint32_t server, uint16_t serverPort,double obsTime) {
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
	sessionData *sessd=findOrAddSessionData(client,clientPort,sd,obsTime);
	if (sessd==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"findOrAddSessionData failed");
		return(0);
	}
	return(sessd);
}
extern long double dabs(double d);
SLDANOM check(double sample,double low,double high, double maxLow, double maxHigh){
	if (sample==0){return(SLD_ZERO);}
	if (sample < maxLow){return(SLD_MAXLOW);}
	if (sample > maxHigh){return(SLD_MAXHIGH);}
	if (sample < low){return(SLD_LOW);}
	if (sample > high){return(SLD_HIGH);}
	return(SLD_OK);
}
double addReading(int32_t serverIP,int16_t serverPort,
		int32_t clientIP,int16_t clientPort,double ptime){
	sessionData *sd=getSessionData(clientIP,clientPort,serverIP,serverPort,
			ptime);
	if (sd == 0) {
		logMessage(stderr,__FUNCTION__,__LINE__,"findOrAddSessionData failed");
		return(0);
	}

	++sd->activeCount;
	sd->lastPacketTime=ptime;
	if (sd->state == 0) {
		sd->startActive = ptime;
	}
	if (sd->startActive == 0){
		logMessage(stderr,__FUNCTION__,__LINE__,"bad ptime");
		exit(-1);
	}
	sd->state=1;
	return(updateScore(&sd->pAvg,(time_t)ptime,1));
}
#define ACTIVETHRESHOLD .001
#define OBSFILE "/tmp/density.csv"
void checkAll(uint32_t serverIP,uint16_t serverPort,sessionData *sd){
	addSldTimeAlert(serverIP,serverPort,sd->clientIP,
		check(sd->lastPacketTime-sd->startActive,TIME03,TIME97,TIME0,
			TIME100));
	addSldCountAlert(serverIP,serverPort,sd->clientIP,
		check(sd->activeCount,COUNT03,COUNT97,COUNT0,COUNT100));
}
time_t startTime=0;
void clearOldSessions(){
	for(size_t i=0;i<SERVDX;++i){
		for(size_t k=0;k<=MAXPORTS;++k){
			size_t j=0;
			if (SERVD[i].SD[k] == 0){continue;}
			for(j=0;j<SERVD[i].DSX[k];++j){
				sessionData *sd=SERVD[i].SD[k][j];
				if (sd->state != 0){break;}
			}
			if (j==SERVD[i].DSX[k]){
				SERVD[i].DSX[k]=0;
				SERVD[i].NSD[k]=0;
				free(SERVD[i].SD[k]);
				SERVD[i].SD[k]=0;
			}
		}
	}
}
size_t updateAll(time_t now){
	static FILE *obs=0;
	size_t cnt=0;
	resetSldAlerts();
	if (obs==0){
		if ((obs=fopen(OBSFILE,"a+e"))==0){
			logMessage(stderr,__FUNCTION__,__LINE__,
				"fopen(%s): %s\n", OBSFILE,strerror(errno));
			exit(-1);
		}
	}
	for(size_t i=0;i<SERVDX;++i){
		for(size_t k=0;k<=MAXPORTS;++k){
			if (SERVD[i].SD[k] == 0){continue;}
			for(size_t j=0;j<SERVD[i].DSX[k];++j){
				sessionData *sd=SERVD[i].SD[k][j];
				double score = getScore(&sd->pAvg,now);
				if (sd->state == 1){
					++cnt;
					if (score < ACTIVETHRESHOLD){
						sd->state=0;
						if (adRecord == true){
							fprintf(obsFile,"%1.6f %lu\n",sd->lastPacketTime-sd->startActive,sd->activeCount);
						}
						if (adDetect == true){
							checkAll(SERVD[i].server,SERVD[i].port,sd);
						}
						fflush(obs);
						sd->lastPacketTime=0;
						sd->startActive=0;
						sd->activeCount=0;
					}
				} else {
					if (adDetect != true){
						continue;
					}
					if ((sd->lastPacketTime-sd->startActive)<=TIME03){
						continue;
					}
					checkAll(SERVD[i].server,SERVD[i].port,sd);
				}
			}
		}
	}
	return(cnt);
}
int parseTcp(char *b,double *pTime,int32_t *sIp,int32_t *dIp,int16_t *sPort,
		int16_t *dPort){
	char a1[SHORTSTRING];
	char a2[SHORTSTRING];
	struct in_addr a;
	char *e=0;
	*pTime=strtod(b,&e);
	if ((e == b) || (*pTime==HUGE_VAL) || (*pTime==-HUGE_VAL)) {
		logMessage(stderr,__FUNCTION__,__LINE__,
				"cannot parse %s\n",b);
		return(-1);
	}
	int rc=sscanf(e,"%s %s",a1,a2);
	if (rc!=2){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"cannot parse %s\n",rc,b);
		return(-1);
	}
	size_t n=strlen(a1)-1;
	while(n>0){
		if (a1[n] == '.'){
			a1[n]='\0';
			*sPort=strToU16(&a1[n+1]);
			break;
		}
		--n;
	}
	n=strlen(a2)-1;
	while(n>0){
		if (a2[n] == '.'){
			a2[n]='\0';
			*dPort=strToU16(&a2[n+1]);
			break;
		}
		--n;
	}
	inet_aton(a1,&a);
	*sIp=ntohl(a.s_addr);
	inet_aton(a2,&a);
	*dIp=ntohl(a.s_addr);
	return(0);
}
int readFromFifo(int fd,sldObs *d){
	ssize_t toRead=sizeof(*d);
	size_t haveRead=0;
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
long long unsigned  OBSIN=0,ANOMALIES=0;
void handleAlerts(double obsTime){
	updateAll(obsTime);
	for(size_t i=0;i<SERVDX;++i){
		char *maxTimeLow=0;
		char *timeLow=0;
		char *timeHigh=0;
		char *maxTimeHigh=0;
		char *timeZero=0;
		char *maxCountLow=0;
		char *countLow=0;
		char *countHigh=0;
		char *maxCountHigh=0;
		char *countZero=0;
		alertDetailsToString(SERVD[i].server,SERVD[i].port,
			&maxTimeLow,&timeLow,&timeHigh,
			&maxTimeHigh,&timeZero,&maxCountLow,&countLow,
			&countHigh,&maxCountHigh,&countZero);
		if(maxTimeLow!=0){
			(*SENDALERT)(SERVD[i].server,SERVD[i].port,SLD_TIME_MAXLOW,maxTimeLow);
			free(maxTimeLow);
		}
		if(timeLow!=0){
			(*SENDALERT)(SERVD[i].server,SERVD[i].port,SLD_TIME_LOW,timeLow);
			free(timeLow);
		}
		if(timeHigh!=0){
			(*SENDALERT)(SERVD[i].server,SERVD[i].port,SLD_TIME_HIGH,timeHigh);
			free(timeHigh);
		}
		if(maxTimeHigh!=0){
			(*SENDALERT)(SERVD[i].server,SERVD[i].port,SLD_TIME_MAXHIGH,maxTimeHigh);
			free(maxTimeHigh);
		}
		if(timeZero!=0){
			(*SENDALERT)(SERVD[i].server,SERVD[i].port,SLD_TIME_ZERO,timeZero);
			free(timeZero);
		}
		if(maxCountLow!=0){
			(*SENDALERT)(SERVD[i].server,SERVD[i].port,SLD_COUNT_MAXLOW,maxCountLow);
			free(maxCountLow);
		}
		if(countLow!=0){
			(*SENDALERT)(SERVD[i].server,SERVD[i].port,SLD_COUNT_LOW,countLow);
			free(countLow);
		}
		if(countHigh!=0){
			(*SENDALERT)(SERVD[i].server,SERVD[i].port,SLD_COUNT_HIGH,countHigh);
			free(countHigh);
		}
		if(maxCountHigh!=0){
			(*SENDALERT)(SERVD[i].server,SERVD[i].port,SLD_COUNT_MAXHIGH,maxCountHigh);
			free(maxCountHigh);
		}
		if(countZero!=0){
			(*SENDALERT)(SERVD[i].server,SERVD[i].port,SLD_COUNT_ZERO,countZero);
			free(countZero);
		}
	}
}
#define ALERTINTERVAL 1
#define CLEARINTERVAL 100
void processTcp(sldObs *sobs){
	static double obsTime=0;
	static double lastAlertTime=0;
	static double lastClearTime=0;
	for (size_t i=0;i<sobs->x;++i){
		++OBSIN;
		addReading(sobs->o[i].serverIP,sobs->o[i].serverPort,
			sobs->o[i].clientIP,
			sobs->o[i].clientPort,
			sobs->o[i].pTime);
		if (obsTime<sobs->o[i].pTime){obsTime=sobs->o[i].pTime;}
	}
	if ((obsTime-lastAlertTime) >= ALERTINTERVAL){
		lastAlertTime=obsTime;
		outputSsv(obsTime);
		handleAlerts(obsTime);
	}
	if ((obsTime-lastClearTime) >= CLEARINTERVAL){
		lastClearTime=obsTime;
		clearOldSessions();
	}
	fprintf(stdout, "%10lu %10llu %10llu\r",time(0),OBSIN,ANOMALIES);
}

#define FIFOWAITPERIOD 1000
void sldMain(callBacks *c){
	startTime=time(0);
	INITALERT=c->initAlert;
	FLUSHALERT=c->flushAlert;
	CLOSEALERT=c->closeAlert;
	SENDALERT=c->sendAlert;
	sldInit();
	initDecayData(&timeAvg);
	initDecayData(&countAvg);
	

	fprintf(stdout,"\e[4m\e[1m	   SLD DETECTOR	      \e[m\n");
	fprintf(stdout, "%10s %10s %10s\n","TIME","OBSIN","ANOMALIES");
	for(int i=0;i<SIGSYS;++i){
		if (i!=SIGSEGV){signal(i,handleSig);}
	}
	while (access("/tmp/SLDFifo",R_OK)==-1){
	    usleep(FIFOWAITPERIOD);
	}
	int fd=0;
	while ((fd = open("/tmp/SLDFifo",
			(unsigned int)O_RDONLY|(unsigned int)O_NONBLOCK|
			(unsigned int)O_CLOEXEC))==-1){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"FIFO open failed %s",strerror(errno));
		while (access("/tmp/SLDFifo",R_OK)==-1){
	    		logMessage(stderr,__FUNCTION__,__LINE__,
					"waiting for fifo");
	    		usleep(FIFOWAITPERIOD);
		}
	}
	while (true){
		fd_set rfds;
		FD_ZERO(&rfds);//NOLINT(hicpp-no-assembler,readability-isolate-declaration)
		FD_SET(fd,&rfds);//NOLINT(hicpp-no-assembler,readability-isolate-declaration,hicpp-signed-bitwise)
		int nfds=fd+1;
		int s=select(nfds,&rfds,0,0,0);
		if ((s==1) && (FD_ISSET(fd,&rfds))){//NOLINT(hicpp-no-assembler,readability-isolate-declaration,hicpp-signed-bitwise)
			sldObs d;
			if (readFromFifo(fd,&d)!=0){
				logMessage(stderr,__FUNCTION__,__LINE__,
					"input fifo closed, stopping");
				break;
			}
			processTcp(&d);
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
		if (!FD_ISSET(fd,&rfds)){//NOLINT(hicpp-no-assembler,readability-isolate-declaration,hicpp-signed-bitwise)
			logMessage(stderr,__FUNCTION__,__LINE__,
					"select fired on unknown fd\n");
				
			continue;
		}
	}
	(*FLUSHALERT)();
	(*CLOSEALERT)();
	close(fd);
}
