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
#include "../common/logMessage.h"
#include "../correlation/alert.h"
#include "defs.h"
#include "sldAlert.h"
#include "sldDetector.h"
#include "sldLog.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
FILE *alertsFile=0;
double correlatorSldFifoFailRate=0;
int correlatorSldFifoFd=0;
void initAlert(){
	if ((alertsFile=fopen(ALERTSFILE,"we"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"%s.%d cannot open %s",
			ALERTSFILE);
		exit(-1);
	}
}
#define FIFOMODE 0666
void initAlertCorr(){
	char fifoName[MEDIUMSTRING];
	initAlert();
	sprintf(fifoName,"/tmp/CORRELATORSLDFifo");
	if (mkfifo(fifoName, FIFOMODE)==-1){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"mkfifo(%s) failed %s",
				fifoName,strerror(errno));
	}
	sleep(1); //give time for fifos to get ready
}
void flushAlertsCorr(){}
void closeAlertCorr(){}
void sendOverFlowAlertCorr(){}
#define DETAILSINCR 100
struct {
	uint32_t clientIP;
	uint32_t serverIP;
	uint16_t serverPort;
	size_t countAnom[SLD_ANOMMAX];
	size_t timeAnom[SLD_ANOMMAX];
} typedef sldAlertDetails;
sldAlertDetails *details;
size_t detailsX=0,detailsN=0,detailsIncr=DETAILSINCR;
void growDetails(){
	sldAlertDetails *t=realloc(details,(detailsN+detailsIncr)*sizeof(sldAlertDetails));
	if (t == 0) {
		logMessage(stderr,__FUNCTION__,__LINE__,"realloc failed");
		exit(-1);
	}
	details=t;
	detailsN+=detailsIncr;
}

size_t COUNTANOM[SLD_ANOMMAX+1];
size_t TIMEANOM[SLD_ANOMMAX+1];
size_t ALERTANOM[SLD_ALERTANOMMAX+1];
char *sldAlertAnomToName(SLDALERTANOM a){
	static char an[SHORTSTRING];
	strcpy(an,"SLD");
	size_t n=strlen(an);
	switch(a){
		case SLD_TIME_OK:
			strncpy(&an[n],"TIMEOK",SHORTSTRING-n);return(an);
		case SLD_TIME_MAXLOW:
		 	strncpy(&an[n],"TIMEMAXLOW",SHORTSTRING-n);return(an);
		case SLD_TIME_LOW:
			strncpy(&an[n],"TIMELOW",SHORTSTRING-n);return(an);
		case SLD_TIME_HIGH:
			strncpy(&an[n],"TIMEHIGH",SHORTSTRING-n);return(an);
		case SLD_TIME_MAXHIGH:
			strncpy(&an[n],"TIMEMAXHIGH",SHORTSTRING-n);return(an);
		case SLD_TIME_ZERO:
			strncpy(&an[n],"TIMEZERO",SHORTSTRING-n);return(an);
		case SLD_COUNT_OK:
			strncpy(&an[n],"COUNTOK",SHORTSTRING-n);return(an);
		case SLD_COUNT_MAXLOW:
			strncpy(&an[n],"COUNTMAXLOW",SHORTSTRING-n);return(an);
		case SLD_COUNT_LOW:
			strncpy(&an[n],"COUNTLOW",SHORTSTRING-n);return(an);
		case SLD_COUNT_HIGH:
			strncpy(&an[n],"COUNTHIGH",SHORTSTRING-n);return(an);
		case SLD_COUNT_MAXHIGH:
			strncpy(&an[n],"COUNTMAXHIGH",SHORTSTRING-n);return(an);
		case SLD_COUNT_ZERO:
			strncpy(&an[n],"COUNTZERO",SHORTSTRING-n);return(an);
		default:
			strncpy(&an[n],"UNK",SHORTSTRING-n);return(an);
	}
	strncpy(&an[n],"BAD",SHORTSTRING-n);
	return(an);
}
void sldAlertToName(char *an,SLDANOM a,size_t n){
	switch(a){
		case SLD_OK: strncpy(an,"OK",n);return;
		case SLD_MAXLOW: strncpy(an,"MAXLOW",n);return;
		case SLD_LOW: strncpy(an,"LOW",n);return;
		case SLD_HIGH: strncpy(an,"HIGH",n);return;
		case SLD_MAXHIGH: strncpy(an,"MAXHIGH",n);return;
		case SLD_ZERO: strncpy(an,"ZERO",n);return;
		default: strncpy(an,"UNK",n);return;
	}
	strncpy(an,"BAD",n);
}
char *sldTimeAlertToName(SLDANOM a){
	static char an[SHORTSTRING];
	strcpy(an,"SLDTIME");
	size_t n=strlen(an);
	sldAlertToName(&an[n],a,SHORTSTRING-n);
	return(an);
}
char *sldCountAlertToName(SLDANOM a){
	static char an[SHORTSTRING];
	strcpy(an,"SLDCOUNT");
	size_t n=strlen(an);
	sldAlertToName(&an[n],a,SHORTSTRING-n);
	return(an);
}
void addDetailsToString(char **s,uint32_t IP,char *anom,size_t count){
	size_t n=0;
	if (*s!=0){n=strlen(*s);}
	char *t=realloc(*s,n+SHORTSTRING);
	if(t==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"realloc failed");
		return;
	}
	sprintf(&t[n],"%08x,%s,%lu\n",IP,anom,count);
	*s=t;
}
void alertDetailsToString(uint32_t serverIP,uint16_t serverPort,
	char **maxTimeLow,char **timeLow,char **timeHigh,
	char **maxTimeHigh, char **timeZero,char **maxCountLow,char **countLow,
	char **countHigh,char **maxCountHigh, char **countZero){
	for(size_t i=0;i<detailsX;++i){
		if ((&details[i])->serverIP != serverIP){continue;}
		if ((&details[i])->serverPort != serverPort){continue;}
		if ((&details[i])->timeAnom[SLD_MAXLOW] >0){
			addDetailsToString(maxTimeLow,(&details[i])->clientIP,
				sldTimeAlertToName(SLD_MAXLOW),
				(&details[i])->timeAnom[SLD_MAXLOW]);
		}
		if ((&details[i])->timeAnom[SLD_LOW] >0){
			addDetailsToString(timeLow,(&details[i])->clientIP,
				sldTimeAlertToName(SLD_LOW),
				(&details[i])->timeAnom[SLD_LOW]);
		}
		if ((&details[i])->timeAnom[SLD_HIGH] >0){
			addDetailsToString(timeHigh,(&details[i])->clientIP,
				sldTimeAlertToName(SLD_HIGH),
				(&details[i])->timeAnom[SLD_HIGH]);
		}
		if ((&details[i])->timeAnom[SLD_MAXHIGH] >0){
			addDetailsToString(maxTimeHigh,(&details[i])->clientIP,
				sldTimeAlertToName(SLD_MAXHIGH),
				(&details[i])->timeAnom[SLD_MAXHIGH]);
		}
		if ((&details[i])->timeAnom[SLD_ZERO] >0){
			addDetailsToString(timeZero,(&details[i])->clientIP,
				sldTimeAlertToName(SLD_ZERO),
				(&details[i])->timeAnom[SLD_ZERO]);
		}
		if ((&details[i])->countAnom[SLD_MAXLOW] >0){
			addDetailsToString(maxCountLow,(&details[i])->clientIP,
				sldCountAlertToName(SLD_MAXLOW),
				(&details[i])->countAnom[SLD_MAXLOW]);
		}
		if ((&details[i])->countAnom[SLD_LOW] >0){
			addDetailsToString(countLow,(&details[i])->clientIP,
				sldCountAlertToName(SLD_LOW),
				(&details[i])->countAnom[SLD_LOW]);
		}
		if ((&details[i])->countAnom[SLD_HIGH] >0){
			addDetailsToString(countHigh,(&details[i])->clientIP,
				sldCountAlertToName(SLD_HIGH),
				(&details[i])->countAnom[SLD_HIGH]);
		}
		if ((&details[i])->countAnom[SLD_MAXHIGH] >0){
			addDetailsToString(maxCountHigh,(&details[i])->clientIP,
				sldCountAlertToName(SLD_MAXHIGH),
				(&details[i])->countAnom[SLD_MAXHIGH]);
		}
		if ((&details[i])->countAnom[SLD_ZERO] >0){
			addDetailsToString(countZero,(&details[i])->clientIP,
				sldCountAlertToName(SLD_ZERO),
				(&details[i])->countAnom[SLD_ZERO]);
		}
	}
}
void resetSldAlerts(){
	for(size_t i=0;i<SLD_ALERTANOMMAX;++i){
		ALERTANOM[i]=0;
	}
	for(size_t i=0;i<SLD_ANOMMAX;++i){
		COUNTANOM[i]=TIMEANOM[i]=0;
	}
	detailsX=0;
}
void dumpSldAlerts(){
	if (detailsX==0){return;}
	logMessage(stderr,__FUNCTION__,__LINE__,"dumping %lu alerts",detailsX);
	for(size_t i=0;i<detailsX;++i){
		logMessage(stderr,__FUNCTION__,__LINE__,
			"%08x,%06u,%05lu,%05lu,%05lu,%05lu,%05lu,%05lu,%05lu,%05lu,%05lu",
			(&details[i])->clientIP,
			(&details[i])->timeAnom[SLD_MAXLOW],
			(&details[i])->timeAnom[SLD_LOW],
			(&details[i])->timeAnom[SLD_HIGH],
			(&details[i])->timeAnom[SLD_MAXHIGH],
			(&details[i])->timeAnom[SLD_ZERO],
			(&details[i])->countAnom[SLD_MAXLOW],
			(&details[i])->countAnom[SLD_LOW],
			(&details[i])->countAnom[SLD_HIGH],
			(&details[i])->countAnom[SLD_MAXHIGH],
			(&details[i])->countAnom[SLD_ZERO]);
	}
}
sldAlertDetails *findIp(uint32_t serverIP,uint16_t serverPort,uint32_t clientIP){
	for(size_t i=0;i<detailsX;++i){
		if ((&details[i])->serverIP!=serverIP){continue;}
		if ((&details[i])->serverPort!=serverPort){continue;}
		if ((&details[i])->clientIP==clientIP){return(&details[i]);}
	}
	return(0);
}

void outputSsv(double obsTime){
	fprintf(ssvFile,"%lu,%1.6f",time(0),obsTime);
	for(size_t i=0;i<SLD_ANOMMAX;++i){
		fprintf(ssvFile,",%lu",TIMEANOM[i]);
	}
	for(size_t i=0;i<SLD_ANOMMAX;++i){
		fprintf(ssvFile,",%lu",COUNTANOM[i]);
	}
	fprintf(ssvFile,"\n");
	fflush(ssvFile);
}
SLDALERTANOM convertTimeToAlertAnom(SLDANOM a){
	switch(a){
		case SLD_OK: return SLD_TIME_OK;
		case SLD_MAXLOW: return SLD_TIME_MAXLOW;
		case SLD_LOW: return SLD_TIME_LOW;
		case SLD_HIGH: return SLD_TIME_HIGH;
		case SLD_MAXHIGH: return SLD_TIME_MAXHIGH;
		case SLD_ZERO: return SLD_TIME_ZERO;
		default: return SLD_ALERTANOMMAX;
	}
	return SLD_ALERTANOMMAX;
}
SLDALERTANOM convertCountToAlertAnom(SLDANOM a){
	switch(a){
		case SLD_OK: return SLD_COUNT_OK;
		case SLD_MAXLOW: return SLD_COUNT_MAXLOW;
		case SLD_LOW: return SLD_COUNT_LOW;
		case SLD_HIGH: return SLD_COUNT_HIGH;
		case SLD_MAXHIGH: return SLD_COUNT_MAXHIGH;
		case SLD_ZERO: return SLD_COUNT_ZERO;
		default: return SLD_ALERTANOMMAX;
	}
	return SLD_ALERTANOMMAX;
}
void addSldTimeAlert(uint32_t server,uint16_t port,uint32_t clientIP,SLDANOM a){
	++ALERTANOM[convertTimeToAlertAnom(a)];
	++TIMEANOM[a];
	if (a==SLD_OK){return;}
	++ANOMALIES;
	sldAlertDetails *d=findIp(server,port,clientIP);
	if (d!=0){
		d->clientIP=clientIP;
		d->serverPort=port;
		d->clientIP=clientIP;
		d->timeAnom[a]++;
		return;
	}
	if (detailsX == detailsN){growDetails();}
	for(size_t i=0;i<SLD_ANOMMAX;++i) {
		(&details[detailsX])->timeAnom[i]=0;
		(&details[detailsX])->countAnom[i]=0;
	}
	(&details[detailsX])->clientIP=clientIP;
	(&details[detailsX])->timeAnom[a]=1;
	(&details[detailsX])->serverIP=server;
	(&details[detailsX])->serverPort=port;
	detailsX++;
}
void addSldCountAlert(uint32_t server,uint16_t port,uint32_t clientIP,SLDANOM a){
	++COUNTANOM[a];
	++ALERTANOM[convertCountToAlertAnom(a)];
	if (a==SLD_OK){return;}
	++ANOMALIES;
	sldAlertDetails *d=findIp(server,port,clientIP);
	if (d!=0){
		d->serverIP=server;
		d->serverPort=port;
		d->clientIP=clientIP;
		d->countAnom[a]++;
		return;
	}
	if (detailsX == detailsN){growDetails();}
	(&details[detailsX])->clientIP=clientIP;
	(&details[detailsX])->serverIP=server;
	(&details[detailsX])->serverPort=port;
	(&details[detailsX])->countAnom[a]=1;
	detailsX++;
}
#define MALLOCPAD 10
void sendSldAlertCorr(uint32_t serverIP,uint32_t serverPort,SLDALERTANOM type, char *details){
	static size_t noWriteError=0;
	static size_t againError=0;
	size_t n=strlen(details);
	SLDALERT *a=malloc(sizeof(SLDALERT)+n+MALLOCPAD);
	if (a == 0){
		logMessage(stderr,__FUNCTION__,__LINE__,"malloc failed");
		return;
	}
	unsigned char *p=(unsigned char *)a;
	strncpy(a->violation,sldAlertAnomToName(type),sizeof(a->violation));
	a->serverIP=ntohl(serverIP);
	a->serverPort=ntohs(serverPort);
	a->alertTime=time(0);
	a->count=ALERTANOM[type];
	strncpy(a->details,details,n+1);
	fprintf(alertsFile,"%08x.%d-----%s-----\n%s\n",
			a->serverIP,a->serverPort,a->violation,a->details);
	size_t toWrite=sizeof(*a)+n;
	size_t haveWritten=0;
	if (correlatorSldFifoFd == 0){
		char fifoName[PATH_MAX];
		correlatorSldFifoFailRate=0;
		sprintf(fifoName,"/tmp/CORRELATORSLDFifo");
		while ((correlatorSldFifoFd=open(fifoName,
			O_WRONLY|O_NONBLOCK|O_CLOEXEC))==-1){ //NOLINT(hicpp-signed-bitwise)
			logMessage(stderr,__FUNCTION__,__LINE__,
				"open %s failed %s",
				fifoName,strerror(errno));
			correlatorSldFifoFd=0;
			return;
		}
	}
	while (write(correlatorSldFifoFd,&toWrite,sizeof(toWrite))==-1){
		if (errno == EAGAIN){continue;}
		return;
	}
	while (toWrite > 0){
		if ((n=write(correlatorSldFifoFd,&p[haveWritten],toWrite))==-1){
			if (errno == EAGAIN){++againError;}
			continue;
		}
		noWriteError++;
		toWrite -= n;
		haveWritten += n;
	}
	correlatorSldFifoFailRate=(double)againError/(double)noWriteError;
	if (correlatorSldFifoFailRate>1){
		logMessage(stderr,__FUNCTION__,__LINE__,
			"correlatorSldFifoFailRate=%g",
			correlatorSldFifoFailRate);
	}
	free(a);
}
