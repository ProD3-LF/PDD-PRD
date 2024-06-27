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
#include "../correlation/alert.h"
#include "defs.h"
#include "history.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
FILE *alertsFile=0;
int correlatorPrdFifoFd=0;
void initAlert(){
	if ((alertsFile=fopen(ALERTSFILE,"we"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"%s.%d cannot open %s",
			ALERTSFILE);
		exit(-1);
	}
}
void flushAlerts(){
	fflush(alertsFile);
}
void closeAlert(){
	fclose(alertsFile);
}

void sendPrdAlert(uint32_t serverIP,uint16_t serverPort,char *violation){
	char serverIPs[SHORTSTRING];
	struct in_addr t;
	serverPort=ntohs(serverPort);
	t.s_addr=serverIP;
        strncpy(serverIPs,inet_ntoa(t),sizeof(serverIPs));
	fprintf(alertsFile,"%lu %15s.%-6u %s\n",
			time(0),serverIPs,ntohs(serverPort),violation);
	(*FLUSHALERT)();
}
void initAlertCorr(){
	char fifoName[80];
	initAlert();
	sprintf(fifoName,"/tmp/CORRELATORPRDFifo");
	if (mkfifo(fifoName, 0666)==-1){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"mkfifo(%s) failed %s",
				fifoName,strerror(errno));
	}
	sleep(1); //give time for fifos to get ready
}
void flushAlertsCorr(){
	flushAlerts();
}
void closeAlertCorr(){
	closeAlert();
	if (correlatorPrdFifoFd != 0){
		close(correlatorPrdFifoFd);
		correlatorPrdFifoFd=0;
	}

}
double correlatorPrdFifoFailRate=0;
void sendPrdAlertCorr(uint32_t serverIP,uint16_t serverPort,char *violation){
	int n;
	size_t histCount;
	PRDALERT *a;
	static size_t noWriteError=0,againError=0;
	size_t toWrite,haveWritten;
	char *d=0;
	if(strcmp(violation,"PRDZERO")==0){
		histCount=ZEROHistN;
		if ((d=malloc(ZEROHistN*50))==0){
			fprintf(stderr,"%s.%d malloc(%lu) failed\n",__FUNCTION__,__LINE__,ZEROHistN*50);
		}
		else outputHist(ZEROHist,&ZEROHistN,d,ZEROHistN*50);
	}
	else if(strcmp(violation,"PRDALLMAX")==0){
		histCount=ALLMAXHistN;
		if ((d=malloc(ALLMAXHistN*50))==0){
			fprintf(stderr,"%s.%d malloc(%lu) failed\n",__FUNCTION__,__LINE__,ALLMAXHistN*50);
		}
		else outputHist(ALLMAXHist,&ALLMAXHistN,d,ALLMAXHistN*50);
	}
	else if(strcmp(violation,"PRDALLSD2")==0){
		histCount=ALL2STDHistN;
		if ((d=malloc(ALL2STDHistN*50))==0){
			fprintf(stderr,"%s.%d malloc(%lu) failed\n",__FUNCTION__,__LINE__,ALL2STDHistN*50);
		}
		else outputHist(ALL2STDHist,&ALL2STDHistN,d,ALL2STDHistN*50);
	}
	else if(strcmp(violation,"PRDMAX")==0){
		histCount=MAXHistN;
		if ((d=malloc(MAXHistN*50))==0){
			fprintf(stderr,"%s.%d malloc(%lu) failed\n",__FUNCTION__,__LINE__,MAXHistN*50);
		}
		else outputHist(MAXHist,&MAXHistN,d,MAXHistN*50);
	}
	else if(strcmp(violation,"PRDSD2")==0){
		histCount=TWOSTDHistN;
		if ((d=malloc(TWOSTDHistN*50))==0){
			fprintf(stderr,"%s.%d malloc(%lu) failed\n",__FUNCTION__,__LINE__,TWOSTDHistN*50);
		}
		else outputHist(TWOSTDHist,&TWOSTDHistN,d,TWOSTDHistN*50);
	}
	sendPrdAlert(serverIP,serverPort,violation);
	toWrite=sizeof(*a)+strlen(d);;
	a=malloc(toWrite+10);
	unsigned char *p=(unsigned char *)a;
	bzero(a,toWrite+10);
	haveWritten=0;
	strncpy(a->violation,violation,sizeof(a->violation));
	a->serverIP=ntohl(serverIP);
	a->serverPort=serverPort;
	a->start=time(0);
	a->alertTime=time(0);
	a->count=histCount;
	strcpy(a->details,d);
	if (correlatorPrdFifoFd == 0){
		char fifoName[80];
		correlatorPrdFifoFailRate=0;
		sprintf(fifoName,"/tmp/CORRELATORPRDFifo");
		while ((correlatorPrdFifoFd=open(fifoName, O_WRONLY|O_NONBLOCK))==-1){
			logMessage(stderr,__FUNCTION__,__LINE__,
				"open %s failed %s",
				fifoName,strerror(errno));
			correlatorPrdFifoFd=0;
			return;
		}
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"d=%s\n",d);
	logMessage(stderr,__FUNCTION__,__LINE__,"a->details=%s\n",a->details);
	while ((n=write(correlatorPrdFifoFd,&toWrite,sizeof(toWrite)))==-1){
			logMessage(stderr,__FUNCTION__,__LINE__,
				"TFB write %s %ld %ld",
				strerror(errno),toWrite,sizeof(toWrite));
			if (errno == EAGAIN) continue;
			return;
	}
	while (toWrite > 0){
		if ((n=write(correlatorPrdFifoFd,&p[haveWritten],toWrite))==-1){
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
	correlatorPrdFifoFailRate=(double)againError/(double)noWriteError;
	if (correlatorPrdFifoFailRate>1) logMessage(stderr,__FUNCTION__,__LINE__,
			"TFB correlatorPrdFifoFailRate=%g",
			correlatorPrdFifoFailRate);
	free(d);
	free(a);
}
