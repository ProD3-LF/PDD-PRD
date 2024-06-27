#/* SPDX-License-Identifier: Apache-2.0 */
#/* Copyright (c) 2012-2023 Applied Communication Sciences
# * (now Peraton Labs Inc.)
# *
# * This software was developed in work supported by the following U.S.
# * Government contracts:
# *
# * HR0011-20-C-0160 HR0011-16-C-0061
# * 
# *
# * Any opinions, findings and conclusions or recommendations expressed in
# * this material are those of the author(s) and do not necessarily reflect
# * the views, either expressed or implied, of the U.S. Government.
# *
# * DoD Distribution Statement A
# * Approved for Public Release, Distribution Unlimited
# *
# * DISTAR Case 38846, cleared November 1, 2023
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# * http://www.apache.org/licenses/LICENSE-2.0
# */
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <messager.h>
#include <ncd.pb-c.h>
#include <pddAlert.pb-c.h>
#include <pthread.h>
#include "../common/util.h"
#include "defs.h"
#include "decay.h"
#include "classifier.h"
#include "pddLog.h"
#include "pddModel.h"
#include "pdd_seq_detector.h"
#define PDDCHECKTIME 30000
#define HALFLIFE 0.5

extern struct in_addr MYADDR;
extern double doubleTimeOfDay();
extern time_t startTime;
char obsFilePath[PATH_MAX]={'\0'};
extern int mkdirEach(char *path);

#ifndef NSEC_PER_SEC
#define NSEC_PER_USEC 1000ULL
#define NSEC_PER_SEC 1000000000ULL
#endif

extern bool adRecord;
extern bool adDetect;

typedef struct
{
	const char *alertTypeString;
	int64_t aggPeriod;
} AlertTypeConfig;

/*Note: order of these must be same as order of corresponding enum*/
AlertTypeConfig alertTypeConfig[] = {
	{"PDDALIEN", 0 * (int64_t) NSEC_PER_SEC},
	{"PDDANOM", 0 * (int64_t) NSEC_PER_SEC},
	{"PDDSTARTSHORT", 0 * (int64_t) NSEC_PER_SEC},
	{"PDDSTARTALIEN", 0 * (int64_t) NSEC_PER_SEC},
	{"PDDSTARTANOM", 0 * (int64_t) NSEC_PER_SEC},
	{"PDDOVERFLOW", 0 * (int64_t) NSEC_PER_SEC},
};
int64_t timespec_to_ns(struct timespec *t){
	return(t->tv_sec*100000000+t->tv_nsec);
}

size_t CAX=0,CAN=0;
DqDecayData avgAll;
#define CAINCR 100
//Need per client
void initAllRate(){
	avgAll.value = 0.0;
	avgAll.lastUpdate = 0.0;
	//need per client
}
struct queuedAlert_ {
	MESSAGE *a;
	struct queuedAlert_ *nextAlert;
} typedef queuedAlert;
struct {
	DqDecayData avg;
	unsigned int IP;
	size_t alerting;
	queuedAlert *nextAlert;
	AggInfo aggInfo[SC_ALERT_MAX];
} typedef clientAvg;
clientAvg **CA;
void initCARate(){
	clientAvg **t;
	if ((t=(clientAvg **)malloc(sizeof(clientAvg *)*CAINCR))==0){
		pd3log_error("malloc");
		return;
	}
	CA=t;
	CAN=CAINCR;
	CAX=0;
}
static void initAggInfo(AggInfo * aggInfo) {
	aggInfo->count = 0;
	aggInfo->detail = NULL;
	aggInfo->firstAlertTime=0;
}
static void resetAggInfo(AggInfo * aggInfo, int64_t now) {
	aggInfo->count = 0;
	if (aggInfo->detail != NULL){
	       	free(aggInfo->detail);
		aggInfo->detail = NULL;
	}
	aggInfo->firstAlertTime=now;
}
static void attachAlertAggInfo(Prod3__PddAlert *alert, unsigned count, int64_t period) {
	unsigned msecs = (unsigned)period/1000000;
	Prod3__AggregationInfo *agg;
	if ((agg=malloc(sizeof(*agg)))==0){
		pd3log_error("malloc failed");
		return;
	}
	prod3__aggregation_info__init(agg);
	agg->count=count;
	agg->msecs=msecs;
	alert->agg_info=agg;
}
void initCA(clientAvg *c,uint32_t client){
	c->IP=client;
	c->avg.value = 0.0;
	c->avg.lastUpdate = 0.0;
	c->nextAlert = 0;
	for(size_t k=0;k<SC_ALERT_MAX;++k){
	 		initAggInfo(&c->aggInfo[k]);
	}
}
int growCA(){
	clientAvg **t;
	if ((t=(clientAvg **)realloc(CA,sizeof(clientAvg *)*(CAN+CAINCR)))==0){
		pd3log_error("realloc failed");
		return(0);
	}
	CA=t;
	CAN+=CAINCR;
	return(1);
}
clientAvg  *insertCA(int rc,int m,uint32_t client){
	int i;
	clientAvg *t;
	if (CAX >= CAN) {
		if (growCA()==0){
			pd3log_error("growCA failed");
			return(0);
		}
	}
	if ((t=(clientAvg *)malloc(sizeof(clientAvg)))==0){
		pd3log_fatal("malloc failed");
		exit(-1);
	}
	initCA(t,client);
	t->IP=client;
	t->avg.value = 0.0;
	t->avg.lastUpdate = 0.0;
	t->nextAlert=0;
	for(size_t k=0;k<SC_ALERT_MAX;++k){
	 	initAggInfo(&t->aggInfo[k]);
	}
	if (rc < 0){
		for(i=CAX;i>m;--i){
			CA[i]=CA[i-1];
		}
		CA[m]=t;
		CAX++;
		return(CA[m]);
	}
	for(i=CAX;i>m+1;--i){
		CA[i]=CA[i-1];
	}
	CA[m+1]=t;
	CAX++;
	return(CA[m+1]);
}
pthread_mutex_t CAMutex;
clientAvg *getCA(unsigned int client){
	int s=0,m=0,e,rc=0;
	clientAvg *ca;
  	pthread_mutex_lock(&CAMutex);
	if (CAX==0){
		if ((CA[0]=(clientAvg *)malloc(sizeof(clientAvg)))==0){
			pd3log_error("malloc");
  			pthread_mutex_unlock(&CAMutex);
			return(0);
		}
		initCA(CA[0],client);
		CAX=1;
  		pthread_mutex_unlock(&CAMutex);
		return(CA[0]);
	}
	e=CAX-1;
	while(s <= e) {
		m = (s+e)/2;
		if ((rc=unsignedComp(client,CA[m]->IP))==0){
  			pthread_mutex_unlock(&CAMutex);
			return(CA[m]);
		}
		if (rc < 0) {
			e = m-1;
		} else {
			s = m+1;
		}
	}
	ca=insertCA(rc,m,client);
  	pthread_mutex_unlock(&CAMutex);
	return(ca);
}
void updateAllRate(time_t start,double count){
	DqDecayAvg avg;
	avg.data = &avgAll;
	avg.decayFactor=HALFLIFE;
	decayAvgNewObservation(&avg,count,start);
	decayAvgNormalized(&avg,start);
}
void updateCARate(DqDecayData *c,time_t start,double count){
	DqDecayAvg avg;
	avg.data = c;
	avg.decayFactor=HALFLIFE;
	decayAvgNewObservation(&avg,count,start);
	decayAvgNormalized(&avg,start);
}
double alert2Rate(DqDecayData *a){
	DqDecayAvg avg;
	struct timeval tv;
	gettimeofday(&tv,0);
	avg.data = a;
	avg.decayFactor=HALFLIFE;
	decayAvgNormalized(&avg,tv.tv_sec);
	return(a->value);
}
pthread_mutex_t alertMutex;
static MESSAGER *m;
extern void *configClient;
void xmitScAlert(MESSAGE *a){
  	pthread_mutex_lock(&alertMutex);
	send_message_for_group(m,a,"pdd");
  	pthread_mutex_unlock(&alertMutex);
	free_message(a);
}
void initMessaging(){
	while ((m=create_messager_with_config(configClient))==0){
		pd3log_error("create_messager() failed, retrying");
		sleep(1);
	}
	supply_messages_for_group(m,"pddAlert", "pdd");
}

MESSAGE *buildPddMessageCommon(uint32_t serverIP,uint32_t serverPort,uint32_t clientIP,uint32_t clientPort,enum SCalert type,uint32_t severity,uint32_t impact,char *seq){
	struct timeval tv;
	MESSAGE *a;
	Prod3__Timestamp *t;
	Prod3__PddAlert *la;
	if ((la=malloc(sizeof(*la)))==0){
		pd3log_error("malloc failed");
		return 0;
	}
	prod3__pdd_alert__init(la);
	la->impact=impact;
	la->severity=severity;
	if ((a=malloc(sizeof(MESSAGE)))==0){
		pd3log_error("malloc failed");
		free(la);
		return 0;
	}
	if ((la->serverip=(Prod3__IPAddress *)malloc(sizeof(Prod3__IPAddress)))==0){
		pd3log_error("malloc failed");
		free(la);
		return 0;
	}
	prod3__ipaddress__init(la->serverip);
	la->serverip->address_case=PROD3__IPADDRESS__ADDRESS_ADDR4;
	la->serverip->addr4=serverIP;
	la->serverport=ntohs(serverPort);
	if ((la->clientip=(Prod3__IPAddress *)malloc(sizeof(Prod3__IPAddress)))==0){
		pd3log_error("malloc failed");
		free(la);
		return 0;
	}
	prod3__ipaddress__init(la->clientip);
	la->clientip->address_case=PROD3__IPADDRESS__ADDRESS_ADDR4;
	la->clientip->addr4=clientIP;
	la->clientport=ntohs(clientPort);
	if ((t=malloc(sizeof(Prod3__Timestamp)))==0){
		pd3log_error("malloc failed");
		free(la);
		free(a);
		return 0;
	}
	prod3__timestamp__init(t);
	gettimeofday(&tv,0);
	t->usec = (uint64_t) tv.tv_sec * 1000000 + (uint64_t) tv.tv_usec;
	la->alerttime = t;
	la->violation = strdup(alertTypeConfig[type].alertTypeString);
	la->sequence = strdup(seq);
	a->proto_message=la;
	a->message_name=strdup("pddAlert");
	return(a);
}
static AggInfo *getAggInfo(clientAvg *c, enum SCalert type, int64_t now) {
	if (type >= 6) {
		pd3log_error("bad type %d",type);
		return NULL;
	}
	AggInfo *aggInfo = &c->aggInfo[type];
	if (aggInfo->count == 0 &&
		(now-aggInfo->firstAlertTime)>=alertTypeConfig[type].aggPeriod){
    /* no agg -- just update time*/
		aggInfo->firstAlertTime = now;
		//pd3log_info("TFB new aggInfo for %08x %d",c->IP,type);
		return NULL;
	}
	return aggInfo;
}
static int updateAggInfoAndCheck(AggInfo *aggInfo,enum SCalert type,
	char *detail,int64_t now) {
  if (aggInfo->count == 0) {
    /* detail for aggregation is set to detail of first alert*/
    if (detail != 0) aggInfo->detail = strdup(detail);
    else pd3log_error("no detail");
  }
  ++aggInfo->count;
  if (now - aggInfo->firstAlertTime >= alertTypeConfig[type].aggPeriod) {
    /* should send*/
    	//pd3log_info("time for %ld %ld %d",(now-aggInfo->firstAlertTime),alertTypeConfig[type].aggPeriod,aggInfo->count);
    return aggInfo->count;
  }
  /* keep aggregating*/
  return -1;
}
int setConImp(int i, int n){
	if (i < n/3) return(PROD3__SEVERITY__LOW);
	if (i < n*(2/3)) return(PROD3__SEVERITY__MEDIUM);
	return(PROD3__SEVERITY__HIGH);
}
DqDecayData avgAll;
clientAvg **CA;
long long unsigned overFlowTime=0;
time_t lastOverflowAlertTime=0;
void sendOverFlowAlert(){
	struct timeval tv;
	static size_t overflowCnt=0;
	MESSAGE *a;
	Prod3__Timestamp *t;
	Prod3__PddAlert *la;
	gettimeofday(&tv,0);
	++overflowCnt;
	pd3log_error("OVERFLOW %ld %ld",
			overflowCnt,tv.tv_sec-lastOverflowAlertTime);
	if (adRecord == true) {
		pd3log_error("TFB OVERFLOW in record aborting");
		exit(-1);
	}
	if ((tv.tv_sec-lastOverflowAlertTime)<30) return;
	overflowCnt=0;
	lastOverflowAlertTime=tv.tv_sec;
	if ((la=malloc(sizeof(*la)))==0){
		pd3log_error("malloc failed");
		return;
	}
	prod3__pdd_alert__init(la);
	la->impact=3;
	la->severity=PROD3__SEVERITY__HIGH;
	if ((a=malloc(sizeof(MESSAGE)))==0){
		pd3log_error("malloc failed");
		free(la);
		return;
	}
	la->serverip=0;
	la->clientip=0;
	if ((t=malloc(sizeof(Prod3__Timestamp)))==0){
		pd3log_error("malloc failed");
		free(la);
		free(a);
		return;
	}
	prod3__timestamp__init(t);
	t->usec = (uint64_t) tv.tv_sec * 1000000 + (uint64_t) tv.tv_usec;
	la->alerttime=t;
	la->violation = strdup(alertTypeConfig[SC_OVERFLOW].alertTypeString);
	la->sequence = strdup("0");
	a->proto_message=la;
	a->message_name=strdup("pddAlert");
	xmitScAlert(a);
}
void xmitQueuedAlerts(long long unsigned now){
	size_t i,alertingClients=0;
	double one,r=0;
	static long long unsigned lastXmit=0;
	if (lastXmit==0) lastXmit=now;
	if ((now - lastXmit)<1000) return;
	lastXmit=now;
  	pthread_mutex_lock(&CAMutex);
	for(i=0;i<CAX;++i){
		if (CA[i]->alerting != 0){
			CA[i]->alerting=0;
		       	++alertingClients;
			r+=alert2Rate(&CA[i]->avg);
		}
	}
	if (alertingClients == 0){
  		pthread_mutex_unlock(&CAMutex);
	       	return;
	}
	r = r/alertingClients;
	for(i=0;i<CAX;++i){
		queuedAlert *qa,*t;
		if ((qa=CA[i]->nextAlert)==0) continue;
		one=alert2Rate(&CA[i]->avg);
		while (qa != 0){
			if (one < r){
				free_message(qa->a);
			}  else {
				xmitScAlert(qa->a);
			}
			t=qa->nextAlert;
			free(qa);
			qa=t;
		}
		CA[i]->nextAlert=0;
	}
  	pthread_mutex_unlock(&CAMutex);
}
void flushAlerts(long long unsigned currentTime){
	static long long unsigned lastFlush=0;
	struct timeval tvnow;
	int64_t now;
	if (lastFlush==0) lastFlush=currentTime;
	if ((currentTime-lastFlush)<1000) return;
	gettimeofday(&tvnow,0);
	now = tvnow.tv_sec*NSEC_PER_SEC+tvnow.tv_usec*NSEC_PER_USEC;
	for(size_t i=0;i<CAX;++i){
		clientAvg *c=CA[i];
		for (int type=0;type<SC_ALERT_MAX;++type){
			AggInfo *aggInfo = &c->aggInfo[type];
			//this function cleans up aggregated
			//alerts for clients no longer alerting
			//so wait for 2 agg periods not just 1
			//and make alerts low severity and confidence
			if (aggInfo->count > 0 &&
				(now - aggInfo->firstAlertTime)
				> 2*alertTypeConfig[type].aggPeriod){
				MESSAGE *alert;
				queuedAlert *qa;
				//pd3log_info("FLUSHING AGGREGATION - count = %d", aggInfo->count);
    				alert = buildPddMessageCommon(0,0,
					c->IP,0,type,0,
					PROD3__SEVERITY__LOW,aggInfo->detail);
				attachAlertAggInfo(alert->proto_message,
						aggInfo->count,
						now - aggInfo->firstAlertTime);
				resetAggInfo(aggInfo, now);
				c->alerting=1;
				if ((qa=malloc(sizeof(queuedAlert)))==0){
					pd3log_error("malloc failed");
					return;
				}
				qa->a=alert;
				qa->nextAlert=c->nextAlert;
				c->nextAlert=qa;
			}
		}
	}
	xmitQueuedAlerts(currentTime);
}
size_t getFifo(uint16_t port){
	return(port % NFIFOS);
}
void sendPddAlert(uint32_t serverIP,uint32_t serverPort,uint32_t clientIP,
	uint16_t clientPort,enum SCalert type,int v[],int n){
	int64_t now;
	struct timeval tvnow;
	queuedAlert *qa;
	MESSAGE *alert;
	char buf[200];
	AggInfo *aggInfo;
	int i,count,confidence,impact;
	if (overFlowTime != 0) return;
	gettimeofday(&tvnow,0);
	now = tvnow.tv_sec*NSEC_PER_SEC+tvnow.tv_usec*NSEC_PER_USEC;
	clientAvg *c;
	/*if (((now/1000000)-sampleChange[clientPort])<TCPIDLETIME){
		//pd3log_info("too soon after sample change to alert on %d",
				//clientPort);
		return;
	}*/
	updateAllRate(tvnow.tv_sec,getFifo(clientPort));
	if ((c=getCA(clientIP))==0){
		pd3log_error("cannot find %08x",clientIP);
	} else {
		updateCARate(&c->avg,tvnow.tv_sec,getFifo(clientPort));
	}
	if (type == SC_ALIEN){
		confidence = impact = PROD3__SEVERITY__HIGH;
	} else {
		confidence = impact = setConImp(n,MAXSCVEC);
	}
	buf[0]='\0';
	for (i=0;i<n;++i){
		int k;
		k = strlen(buf);
		sprintf(&buf[k],"%d ",v[i]);
	}
	if ((aggInfo=getAggInfo(c,type,now))){
		count = updateAggInfoAndCheck(aggInfo, type, buf, now);
		if (count < 0) {
	  		//pd3log_info("FINE: TOO FAST -- AGGREGATING");
	  		return;
		}
		/*pd3log_info("INFO: DONE AGGREGATING -- count = %d",
				aggInfo->count);*/
	  	if ((alert=buildPddMessageCommon(serverIP,serverPort,clientIP,clientPort,type,confidence,impact,
						buf))==0) return;
	  	attachAlertAggInfo(alert->proto_message, aggInfo->count, now - aggInfo->firstAlertTime);
		//pd3log_info("attached -- count = %d",aggInfo->count);
	  	resetAggInfo(aggInfo, now);
	} else {
	  /* not aggregating*/
		alert = buildPddMessageCommon(serverIP,serverPort,clientIP,clientPort,type,confidence,impact,buf);
		if (alert == 0) return;
	}
	if ((qa=malloc(sizeof(queuedAlert)))==0){
		pd3log_error("malloc failed");
		return;
	}
	c->alerting=1;
	qa->a=alert;
	qa->nextAlert=c->nextAlert;
	c->nextAlert=qa;
}
#define NRA 32
struct {
	uint32_t serverIP;
	uint16_t serverPort;
	char violation[64];
	Prod3__Severity s;
	long long unsigned int lastReport;
} typedef alertRecord;
static alertRecord AR[NRA];
static size_t ARX=0;
void clearOldReportedAlerts(long long unsigned int now){
	for(size_t i=0;i<ARX;++i){
		if ((now-AR[i].lastReport)>PDDCHECKTIME*2){
			AR[i].serverIP=0;
			AR[i].serverPort=0;
			AR[i].violation[0]='\0';
			AR[i].lastReport=0;
			for(size_t j=i+1;j<ARX;++j){
				AR[i]=AR[j];
			}
			--ARX;
			--i;
		}
	}
}

bool isRedundantAlert(int32_t serverIP,uint16_t serverPort,char *violation,Prod3__Severity s){
	long long unsigned int now = msecTime();
	for (size_t i=0;i<ARX;++i) {
		if ((AR[i].serverIP==serverIP) &&
				(AR[i].serverPort==serverPort) &&
				(AR[i].s == s) &&
				(strcmp(AR[i].violation,violation)==0)){
			if ((now - AR[i].lastReport) > PDDCHECKTIME){
			       AR[i].lastReport = now;
		       		return(false);
			}
			return(true);
		}
	}
	clearOldReportedAlerts(now);
	if (ARX == NRA){
		pd3log_info("too many unique reported lerts");
		return(false);
	}
	AR[ARX].serverIP=serverIP;
	AR[ARX].serverPort=serverPort;
	strcpy(AR[ARX].violation,violation);
	AR[ARX].s=s;
	AR[ARX].lastReport = now;
	++ARX;
	return(false);
}
void initAlert(){
	pthread_mutex_init(&alertMutex, NULL);
	pthread_mutex_init(&CAMutex, NULL);
	lastOverflowAlertTime=0;
	initAllRate();
	initCARate();
	initMessaging();
}
