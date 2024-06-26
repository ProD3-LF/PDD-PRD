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
 * DISTAR Case 39809, cleared May 22, 2024
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include "defs.h"
#include "../common/pddObs.h"
#include "../common/prdObs.h"
#include "../common/config.h"
#include "../common/logMessage.h"
#define NSERVD 2
extern double pddFifoFailRate[];
static nodeCwCnt NC[NSERVD];
static size_t NCX=0;
extern int pddFifoFd[NFIFOS];
extern int prdFifoFd;
static long long unsigned int pddObsTime=0;
void updatePddObsTime(double t){
	long long unsigned int c=ceil(t*1000);
	if (c > pddObsTime) pddObsTime=c;
}
long long unsigned int getPddObsTime(){
	return(pddObsTime);
}

void initSldFifo(){
	char fifoName[80];
	sprintf(fifoName,"/tmp/SLDFifo");
	if (prdFifoFd== 0){
		if (mkfifo(fifoName, 0666)==-1){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"mkfifo(%s) failed %s",
					fifoName,strerror(errno));
		}
	}
	sleep(1); //give time for fifos to get ready
}
void initPrdFifo(){
	char fifoName[80];
	sprintf(fifoName,"/tmp/PRDFifo");
	if (prdFifoFd== 0){
		if (mkfifo(fifoName, 0666)==-1){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"mkfifo(%s) failed %s",
					fifoName,strerror(errno));
		}
	}
	sleep(1); //give time for fifos to get ready
}

void initPddFifos(){
	for(size_t i=0;i<NFIFOS;++i){
		char fifoName[80];
		sprintf(fifoName,"/tmp/PDDFifo%ld",i);
		if (mkfifo(fifoName, 0666)==-1){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"mkfifo(%s) failed %s",
					fifoName,strerror(errno));
		}
		pddFifoFd[i]=-1;
	}
	//Note do not open fifos for writing now because open will
	//fail until detector first opens them for reading.  Open fifos
	//as needed when writing to them.
}

uint16_t knownSldServerPort[MAXSERVER];
in_addr_t knownSldServerIP[MAXSERVER];
size_t knownSldServerN=0;
uint16_t knownPddServerPort[MAXSERVER];
in_addr_t knownPddServerIP[MAXSERVER];
size_t knownPddServerN=0;
#define MAXSERVER 10
uint16_t knownPrdServerPort[MAXSERVER];
in_addr_t knownPrdServerIP[MAXSERVER];
size_t knownPrdServerN=0;
bool knownSldServer(uint16_t p,in_addr_t ip){
	for(size_t i=0;i<knownSldServerN;++i){
		if ((knownSldServerPort[i]==p) && (knownSldServerIP[i]==ip))
			return(true);
	}
	return false;
}
bool knownPrdServer(uint16_t p,in_addr_t ip){
	for(size_t i=0;i<knownPrdServerN;++i){
		if ((knownPrdServerPort[i]==p) && (knownPrdServerIP[i]==ip))
			return(true);
	}
	return false;
}

bool knownPddServer(uint16_t p,in_addr_t ip){
	for(size_t i=0;i<knownPddServerN;++i){
		if ((knownPddServerPort[i]==p) && (knownPddServerIP[i]==ip))
			return(true);
	}
	return false;
}
void addKnownSldServer(uint16_t p,in_addr_t ip){
	knownSldServerPort[knownSldServerN]=ntohs(p);
	knownSldServerIP[knownSldServerN++]=ip;
}
void addKnownPddServer(uint16_t p,in_addr_t ip){
	knownPddServerPort[knownPddServerN]=ntohs(p);
	knownPddServerIP[knownPddServerN++]=ip;
}
void addKnownPrdServer(uint16_t p,in_addr_t ip){
	logMessage(stderr,__FUNCTION__,__LINE__,"new server %x.%u at %lu",ip,p,knownPrdServerN);
	knownPrdServerPort[knownPrdServerN]=ntohs(p); //leave in network order for
						//faster inline compare
	knownPrdServerIP[knownPrdServerN++]=ip;
}
void resetCwCnts(nodeCwCnt *nc){
	for(size_t i=0;i<MAXCW;++i){
		nc->prdCountIn[i]=nc->prdCountOut[i]=0;
	}
	nc->prdTotal=0;
}
void addNC(uint32_t ip,uint16_t port){
	for(size_t i=0;i<NCX;++i){
		if ((NC[i].serverIP==ip) && (NC[i].serverPort==port)){
			logMessage(stderr,__FUNCTION__,__LINE__,"%x.%d already exists",ip,port);
			return;
		}
	}
	NC[NCX].serverIP=ip;
	NC[NCX].serverPort=port;
	NC[NCX].lastOutputTime=0;
	logMessage(stderr,__FUNCTION__,__LINE__,"new NC for %x.%d at %lu",ip,port,NCX);
	resetCwCnts(&NC[NCX]);
	++NCX;
	return;
}
void setupNC(){
	for(size_t i=0;i<knownPrdServerN;++i){
		addNC(knownPrdServerIP[i],knownPrdServerPort[i]);
	}
}

nodeCwCnt *getNC(uint32_t ip,uint16_t port){
	for(size_t i=0;i<NCX;++i){
		if ((NC[i].serverIP==ip) && (NC[i].serverPort==port)){
			return(&NC[i]);
		}
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"NC for %x.%d does not exist",ip,port);
	return(0);
}
void setupPrdServers(char *s){
	size_t n=strlen(s);
	char *t=s;
	in_addr_t ip;
	uint16_t port;
	for(size_t i=0;i<n;++i){
		size_t j;
		if (s[i]==':'){
			s[i]='\0';
			ip=inet_addr(t);
			port=atoi(&s[i+1]);
			addKnownPrdServer(port,ip);
			s[i]=':';
			for(j=i+1;j<n;++j){
				if (s[j] == ' ') break;
			}
			i=j;
			t=&s[j+1];
		}
	}
	setupNC();
	logMessage(stderr,__FUNCTION__,__LINE__,"Servers are set up");
}

void setupSldServers(char *s){
	size_t n=strlen(s);
	char *t=s;
	in_addr_t ip;
	uint16_t port;
	for(size_t i=0;i<n;++i){
		size_t j;
		if (s[i]==':'){
			s[i]='\0';
			ip=inet_addr(t);
			port=atoi(&s[i+1]);
			addKnownSldServer(port,ip);
			s[i]=':';
			for(j=i+1;j<n;++j){
				if (s[j] == ' ') break;
			}
			i=j;
			t=&s[j+1];
		}
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"Servers are set up");
}
	
void sldConfig(){
	char *t=0;
	FILE *configFile;
	if ((configFile=fopen(CONFIGFILE,"r+"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"cannot open %s",CONFIGFILE);
		exit(-1);
	}
	if ((t=get_config_string(configFile,"sldservers"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"no servers in configFile");
		exit(-1);
	}
	setupSldServers(t);
}
void setupPddServers(char *s){
	size_t n=strlen(s);
	char *t=s;
	in_addr_t ip;
	uint16_t port;
	for(size_t i=0;i<n;++i){
		size_t j;
		if (s[i]==':'){
			s[i]='\0';
			ip=inet_addr(t);
			port=atoi(&s[i+1]);
			addKnownPddServer(port,ip);
			s[i]=':';
			for(j=i+1;j<n;++j){
				if (s[j] == ' ') break;
			}
			i=j;
			t=&s[j+1];
		}
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"Servers are set up");
}
	
void prdConfig(){
	char *t=0;
	FILE *configFile;
	if ((configFile=fopen(CONFIGFILE,"r+"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"cannot open %s",CONFIGFILE);
		exit(-1);
	}
	if ((t=get_config_string(configFile,"prdservers"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"no servers in configFile");
		exit(-1);
	}
	setupPrdServers(t);
}
void pddConfig(){
	char *t=0;
	FILE *configFile;
	if ((configFile=fopen(CONFIGFILE,"r+"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"cannot open %s",CONFIGFILE);
		exit(-1);
	}
	if ((t=get_config_string(configFile,"pddservers"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"no servers in configFile");
		exit(-1);
	}
	setupPddServers(t);
	initPddFifos();
	initPrdFifo();
	initSldFifo();
	initSldObs();
	initBucket();
}
