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
#include "../common/logMessage.h"
#include "defs.h"
#include "pdd_seq_detector.h"
#include "../correlation/alert.h"
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#ifndef NSEC_PER_SEC
#define NSEC_PER_USEC 1000ULL
#define NSEC_PER_SEC 1000000000ULL
#endif
#ifndef USEC_PER_SEC
#define USEC_PER_SEC 1000000ULL
#endif
FILE *alertsFile=0;
time_t lastOverflowAlertTime=0;
double correlatorPddFifoFailRate=0;
int correlatorPddFifoFd=0;
typedef struct {
	const char *alertTypeString;
	int64_t aggPeriod;
} AlertTypeConfig;
char *alertTypeString(enum SCalert t){
	switch(t){
		case SC_ALIEN: return("PDDALIEN");
		case SC_ANOM: return("PDDANOM");
		case SC_START_SHORT: return("PDDSTARTSHORT");
		case SC_START_ALIEN: return("PDDSTARTALIEN");
		case SC_START_ANOM: return("PDDSTARTANOM");
		case SC_OVERFLOW: return("PDDOVERFLOW");
		default: return("UNKNOWN");
	}
	return("UNKNOWN");
}
void initAlert(){
	if ((alertsFile=fopen(ALERTSFILE,"we"))==0){
       		logMessage(stderr,__FUNCTION__,__LINE__,"%s.%d cannot open %s",
       	                  ALERTSFILE);
		exit(-1);
	}
	lastOverflowAlertTime=0;
}
void closeAlert(){
	fclose(alertsFile);
}
void flushAlerts(){
	fflush(alertsFile);
}
void sendPddAlert(uint32_t serverIP,uint32_t serverPort,uint32_t clientIP,
	uint16_t clientPort,enum SCalert type,int v[],int n){
	char clientIPs[SHORTSTRING],serverIPs[SHORTSTRING];
	char d[512];
	d[0]=0;
	struct in_addr t;
	for (int i=0;i<n;++i){
		char t[10];
		sprintf(t,"%d ",v[i]);
		strcat(d,t);
	}
	t.s_addr=serverIP;
	strncpy(serverIPs,inet_ntoa(t),sizeof(serverIPs));
	t.s_addr=clientIP;
	strncpy(clientIPs,inet_ntoa(t),sizeof(clientIPs));
	fprintf(alertsFile,"%lu %15s.%-6u %15s.%-6u %-8s %2d %s\n",
			time(0),clientIPs,clientPort,serverIPs,
			ntohs(serverPort),alertTypeString(type),n,d);
}
void initAlertCorr(){
	char fifoName[80];
	initAlert();
	sprintf(fifoName,"/tmp/CORRELATORPDDFifo");
	if (mkfifo(fifoName, 0666)==-1){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"mkfifo(%s) failed %s",
				fifoName,strerror(errno));
	}
	sleep(1); //give time for fifos to get ready
}
void flushAlertsCorr(){
}
void closeAlertCorr(){
	closeAlert();
	if (correlatorPddFifoFd != 0){
		close(correlatorPddFifoFd);
		correlatorPddFifoFd=0;
	}

}
void sendPddAlertCorr(uint32_t serverIP,uint32_t serverPort,uint32_t clientIP,
	uint16_t clientPort,enum SCalert type,int v[],int n){
	PDDALERT a;
	static size_t noWriteError=0,againError=0;
	size_t toWrite,haveWritten;
	unsigned char *p=(unsigned char *)&a;
	bzero(&a,sizeof(a));
	sendPddAlert(serverIP,serverPort,clientIP,clientPort,type,v,n);
	a.clientIP=ntohl(clientIP);
	a.serverIP=ntohl(serverIP);
	a.clientPort=clientPort;
	a.serverPort=ntohs(serverPort);
	strcpy(a.violation,alertTypeString(type));
	a.start=time(0);
	a.alertTime=time(0);
	a.count=1;
	a.details[0]='\0';
	toWrite=sizeof(a);
	haveWritten=0;
	if (correlatorPddFifoFd == 0){
		char fifoName[80];
		correlatorPddFifoFailRate=0;
		sprintf(fifoName,"/tmp/CORRELATORPDDFifo");
		while ((correlatorPddFifoFd=open(fifoName, O_WRONLY|O_NONBLOCK))==-1){
			logMessage(stderr,__FUNCTION__,__LINE__,
				"open %s failed %s",
				fifoName,strerror(errno));
			correlatorPddFifoFd=0;
			return;
		}
	}
	while ((n=write(correlatorPddFifoFd,&toWrite,sizeof(toWrite)))==-1){
			logMessage(stderr,__FUNCTION__,__LINE__,
				"TFB write %s %ld %ld",
				strerror(errno),toWrite,sizeof(toWrite));
			if (errno == EAGAIN) continue;
			return;
	}
	while (toWrite > 0){
		if ((n=write(correlatorPddFifoFd,&p[haveWritten],toWrite))==-1){
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
	correlatorPddFifoFailRate=(double)againError/(double)noWriteError;
	if (correlatorPddFifoFailRate>1) logMessage(stderr,__FUNCTION__,__LINE__,
			"TFB correlatorPddFifoFailRate=%g",
			correlatorPddFifoFailRate);
}

/*Note: order of these must be same as order of corresponding enum*/
AlertTypeConfig alertTypeConfig[] = {
	{"PDDALIEN", 0 * (int64_t) NSEC_PER_SEC},
	{"PDDANOM", 0 * (int64_t) NSEC_PER_SEC},
	{"PDDSTARTSHORT", 0 * (int64_t) NSEC_PER_SEC},
	{"PDDSTARTALIEN", 0 * (int64_t) NSEC_PER_SEC},
	{"PDDSTARTANOM", 0 * (int64_t) NSEC_PER_SEC},
	{"PDDOVERFLOW", 0 * (int64_t) NSEC_PER_SEC},
};
#define ALERTSIZE LONGSTRING
#define OVERFLOWALERTTIME 30
void sendOverFlowAlert(){
	struct timeval tv;
	static size_t overflowCnt=0;
	gettimeofday(&tv,0);
	++overflowCnt;
	logMessage(stderr,__FUNCTION__,__LINE__,"OVERFLOW %ld %ld",
			overflowCnt,tv.tv_sec-lastOverflowAlertTime);
	if (adRecord == true) {
		logMessage(stderr,__FUNCTION__,__LINE__,"TFB OVERFLOW in record aborting");
		exit(-1);
	}
	if ((tv.tv_sec-lastOverflowAlertTime)<OVERFLOWALERTTIME){return;}
	overflowCnt=0;
	lastOverflowAlertTime=tv.tv_sec;
	fprintf(alertsFile,"%llu pddAlert %08x.%u %08x.%u %s %d %d",
	 	(uint64_t)tv.tv_sec * USEC_PER_SEC + (uint64_t)tv.tv_usec,
		0,0,0,0,
	        alertTypeConfig[SC_OVERFLOW].alertTypeString,
		3,PROD3__SEVERITY__HIGH);
}
