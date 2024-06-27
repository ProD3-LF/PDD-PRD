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
#include <prdAlert.pb-c.h>
#include "defs.h"
#include "prdLog.h"
#include "history.h"
static MESSAGER *m;
void initMessaging(){
	if (configClient==0){
		pd3log_error("configClient not initialized");
		exit(-1);
	}
        while ((m=create_messager_with_config(configClient))==0){
		pd3log_error("create_messager() failed, retrying");
		sleep(1);
	}
	supply_messages_for_group(m,"prdAlert", "prd");
}
MESSAGE *buildPrdMessageCommon(uint32_t serverIP,uint32_t serverPort,char *violation,uint32_t severity,size_t alertCount, long long unsigned int alertPeriod,char *details){
	struct timeval tv;
	MESSAGE *a;
	Prod3__Timestamp *t;
	Prod3__PrdAlert *la;
	if ((la=malloc(sizeof(*la)))==0){
		pd3log_error("malloc failed");
		return 0;
	}
	prod3__prd_alert__init(la);
	switch(severity){
		case 1: la->severity=PROD3__SEVERITY__LOW; break;
		case 2: la->severity=PROD3__SEVERITY__MEDIUM; break;
		case 3: la->severity=PROD3__SEVERITY__HIGH; break;
		default:
			pd3log_error("bad severity %d using HIGH",severity);
			la->severity=PROD3__SEVERITY__HIGH;
	}
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
	Prod3__AggregationInfo *agg;
	if ((agg=malloc(sizeof(*agg)))==0){
                pd3log_error("malloc failed");
		free(la);
		free(a);
		free(t);
		return 0;
        }
        prod3__aggregation_info__init(agg);
        agg->count=alertCount;
        agg->msecs=alertPeriod;
        la->agg_info=agg;
	la->violation = strdup(violation);
	if (details != 0) la->details=strdup(details);
	else la->details=strdup("NODETAILS");
	a->proto_message=la;
	return(a);
}
#define NRA 32
struct {
	uint32_t serverIP;
	uint16_t serverPort;
	char violation[64];
	Prod3__Severity s;
	long long unsigned int lastReport;
	size_t alertCount;

} typedef alertRecord;
alertRecord AR[NRA];

size_t ARX=0;
void clearOldReportedAlerts(long long unsigned int now);
bool isRedundantAlert(int32_t serverIP,uint16_t serverPort,char *violation,Prod3__Severity s,size_t *alertCount, long long unsigned int *alertPeriod){
	long long unsigned int now = msecTime();
	for (size_t i=0;i<ARX;++i) {
		if ((AR[i].serverIP==serverIP) &&
				(AR[i].serverPort==serverPort) &&
				(AR[i].s == s) &&
				(strcmp(AR[i].violation,violation)==0)){
			 ++AR[i].alertCount;
			if ((now - AR[i].lastReport) > PRDALERTINTERVAL){
			       *alertPeriod=now-AR[i].lastReport;
			       *alertCount=AR[i].alertCount;
			       AR[i].lastReport = now;
			       AR[i].alertCount = 0;
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
	AR[ARX].alertCount = 0;
	*alertPeriod=0;
	*alertCount=1;
	++ARX;
	return(false);
}
void sendPrdAlert(uint32_t serverIP,uint16_t serverPort,char *violation){
	Prod3__Severity s;
	size_t alertCount;
	long long unsigned int alertPeriod;
        pd3log_info("might send");
	if (strstr(violation,"SD2")==0){s=PROD3__SEVERITY__HIGH;}
	else {s=PROD3__SEVERITY__LOW;}
	if (isRedundantAlert(serverIP,serverPort,violation,s,&alertCount,&alertPeriod)){
        	pd3log_info("not send");
	       	return;
	}
	serverPort=ntohs(serverPort);
	char *d=0;
	if(strcmp(violation,"PRDZERO")==0){
		if ((d=malloc(ZEROHistN*50))==0){
			fprintf(stderr,"%s.%d malloc(%lu) failed\n",__FUNCTION__,__LINE__,ZEROHistN*50);
		}
		else outputHist(ZEROHist,&ZEROHistN,d,ZEROHistN*50);
	}
	else if(strcmp(violation,"PRDALLMAX")==0){
		if ((d=malloc(ALLMAXHistN*50))==0){
			fprintf(stderr,"%s.%d malloc(%lu) failed\n",__FUNCTION__,__LINE__,ALLMAXHistN*50);
		}
		else outputHist(ALLMAXHist,&ALLMAXHistN,d,ALLMAXHistN*50);
	}
	else if(strcmp(violation,"PRDALLSD2")==0){
		if ((d=malloc(ALL2STDHistN*50))==0){
			fprintf(stderr,"%s.%d malloc(%lu) failed\n",__FUNCTION__,__LINE__,ALL2STDHistN*50);
		}
		else outputHist(ALL2STDHist,&ALL2STDHistN,d,ALL2STDHistN*50);
	}
	else if(strcmp(violation,"PRDMAX")==0){
		if ((d=malloc(MAXHistN*50))==0){
			fprintf(stderr,"%s.%d malloc(%lu) failed\n",__FUNCTION__,__LINE__,MAXHistN*50);
		}
		else outputHist(MAXHist,&MAXHistN,d,MAXHistN*50);
	}
	else if(strcmp(violation,"PRDSD2")==0){
		if ((d=malloc(TWOSTDHistN*50))==0){
			fprintf(stderr,"%s.%d malloc(%lu) failed\n",__FUNCTION__,__LINE__,TWOSTDHistN*50);
		}
		else outputHist(TWOSTDHist,&TWOSTDHistN,d,TWOSTDHistN*50);
	}
	MESSAGE *a=buildPrdMessageCommon(serverIP,serverPort,
			violation,s,alertCount,alertPeriod,d);
	if (d!=0) free(d);
	if (a == 0){
		pd3log_error("buildPrdMessageCommon failed");
		return;
	}
        pd3log_info("send");
	a->message_name=strdup("prdAlert");
	send_message_for_group(m,a,"prd");
	free_message(a);
	return;
}
void clearOldReportedAlerts(long long unsigned int now){
	for(size_t i=0;i<ARX;++i){
		if ((now-AR[i].lastReport)>PRDCHECKTIME*2){
			if (AR[i].alertCount != 0){
				pd3log_error("clearing unreported alerts");
				sendPrdAlert(AR[i].serverIP,AR[i].serverPort,AR[i].violation);
			}
			AR[i].serverIP=0;
			AR[i].serverPort=0;
			AR[i].violation[0]='\0';
			AR[i].lastReport=0;
			AR[i].alertCount=0;
			for(size_t j=i+1;j<ARX;++j){
				AR[i]=AR[j];
			}
			--ARX;
			--i;
		}
	}
}
void closeAlert(){
	pd3log_info("close");
}
void flushAlerts(){
	pd3log_info("flush");
}
void initAlert(){
	initMessaging();
}
