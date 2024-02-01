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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#ifndef NSEC_PER_SEC
#define NSEC_PER_USEC 1000ULL
#define NSEC_PER_SEC 1000000000ULL
#endif
#ifndef USEC_PER_SEC
#define USEC_PER_SEC 1000000ULL
#endif
FILE *alertsFile=0;
time_t lastOverflowAlertTime=0;
void initAlert(){
	if ((alertsFile=fopen(ALERTSFILE,"we"))==0){
       		logMessage(stderr,__FUNCTION__,__LINE__,"%s.%d cannot open %s",
       	                  ALERTSFILE);
		exit(-1);
	}
	lastOverflowAlertTime=0;
}
typedef struct {
	const char *alertTypeString;
	int64_t aggPeriod;
} AlertTypeConfig;
char *alertTypeString(enum SCalert t){
	switch(t){
		case SC_ALIEN: return("ALIEN");
		case SC_ANOM: return("ANOM");
		case SC_START_SHORT: return("STARTSHORT");
		case SC_START_ALIEN: return("STARTALIEN");
		case SC_START_ANOM: return("STARTANOM");
		case SC_OVERFLOW: return("OVERFLOW");
		default: return("UNKNOWN");
	}
	return("UNKNOWN");
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
