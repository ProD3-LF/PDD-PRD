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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "defs.h"
#include "../common/pddObs.h"
#include "../common/logMessage.h"
#include "../common/util.h"
static long long unsigned int sensorStartTime=0;
static long long unsigned int firstObsTime=0;
size_t pddObsCnt=0;
size_t prdObsCnt=0;
void prdSensor(in_addr_t serverIP,uint32_t serverPort,int8_t direction,uint8_t flag,double obsTime){
	handlePRDObs(obsTime,serverIP,serverPort,flag,direction);
}
void pddSensor(in_addr_t serverIP,in_addr_t clientIP,uint32_t serverPort,
	uint32_t clientPort,int8_t direction,uint8_t flag,double obsTime){
	++pddObsCnt;
	handlePDDObs(serverIP,clientIP,serverPort,clientPort,flag,direction);
}
long long unsigned int relativeObsTime,relativeRealTime;
long long unsigned int speedFactor=1;
void adjustSpeed(long long unsigned int obsTime){
	long long unsigned int now;
	if (firstObsTime==0) firstObsTime=obsTime;
	now=msecTime();
	if (sensorStartTime==0) sensorStartTime=now;
	relativeObsTime=(obsTime-firstObsTime);
	relativeRealTime=now-sensorStartTime;
	if (relativeObsTime/speedFactor==relativeRealTime){
		return;
	}
	if (relativeObsTime/speedFactor>relativeRealTime) {
		logMessage(stderr,__FUNCTION__,__LINE__,"playback is fast %llu\n",(relativeObsTime/speedFactor-relativeRealTime)*1000);
		usleep(((relativeObsTime/speedFactor-relativeRealTime)*1000));
		return;
	}
}
//ARGS:
//	--pcapFile: if present playback from pcap file.
//	--speedFactor:  if present set speedFactor to value else 
//		speedFactor is 1
extern size_t obsSent;
int main(int argc, char *argv[]){
	FILE *t;
	char *pcapFile=0,b[512];
	time_t lastScreenUpdate=0;
	static long long unsigned int obsCnt=0;
	for(size_t i=1;i<argc;++i){
		if (strcmp("--pcapFile",argv[i])==0){
			pcapFile=argv[++i];
			continue;
		}
		if (strcmp("--speedFactor",argv[i])==0){
			speedFactor=atoi(argv[++i]);
			continue;
		}
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"pcapFile %s speedFactor\n",
			pcapFile,speedFactor);
	pddConfig();
	prdConfig();
	if (pcapFile == 0) {
		snprintf(b,512,"./tcpdumpCommand");
	} else {
		snprintf(b,512,"tcpdump -nn -tt -s 100 -r %s 2>/tmp/tcpdump.err",pcapFile);
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"%s",b);
	if ((t=popen(b,"r"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"popen(%s): %s",b,strerror(errno));
		exit(-1);
	}
	fprintf(stdout,"\e[4m\e[1m                 SENSOR               \e[m\n");
	fprintf(stdout,"\033[H\033[J");
	fprintf(stdout,"\e[4m\e[1m                 SENSOR               \e[m\n");
	fprintf(stdout,"%10s %8s %7s %10s\n",
			"Time","RealTime","ObsTime","OBS");
	if (speedFactor<=0) {
		logMessage(stderr,__FUNCTION__,__LINE__,"bad speed factor");
		exit(-1);
	}
	while (fgets(b,512,t)){
		in_addr_t serverPDDIP,clientPDDIP,serverPRDIP;
		uint32_t serverPDDPort,clientPDDPort,serverPRDPort;
		int8_t direction;
		uint8_t flag;
		double obsTime;
		if (parseTcpLine(b,&obsTime,&serverPDDIP,&clientPDDIP,
				&serverPDDPort, &clientPDDPort,
				&serverPRDIP, &serverPRDPort, &flag,
				&direction)!=0){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"parse failed %s\n",b);
			continue;
		}
		++obsCnt;
		adjustSpeed((long long unsigned int)(obsTime*1000));
		pddSensor(serverPDDIP,clientPDDIP,serverPDDPort,clientPDDPort,direction,flag,obsTime);
		prdSensor(serverPRDIP,serverPRDPort,direction,flag,obsTime);
		if (time(0)-lastScreenUpdate>=1){
	                fprintf(stdout, "%10llu %8llu %7llu %10llu\r",
                        msecTime()/1000,relativeRealTime/1000,
			relativeObsTime/1000,obsCnt);
                	lastScreenUpdate=time(0);
                	fflush(stdout);
		}
	}
	pclose(t);
	flushBuckets();
	logMessage(stderr,__FUNCTION__,__LINE__,"buckets flushed");
	sleep(5);
	closeFifos();
	sleep(5);
	logMessage(stderr,__FUNCTION__,__LINE__,"fifos closed");
	logMessage(stderr,__FUNCTION__,__LINE__,"obsSent=%lu",obsSent);

}
