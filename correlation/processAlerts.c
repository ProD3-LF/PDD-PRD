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
#include "alert.h"
#include "ar.h"
#include "baseProg.h"
#include "classifier.h"
#include "defs.h"
#include "formula.h"
#include "scenario.h"
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int AGETHRESHOLD=AGEFACTOR;
extern void classifyAda(baseProg *b);

extern struct sockaddr_in frameworkAddr;
extern double alertType2Age(baseProg *b,int alertType,time_t now);
extern double alertType2Rate(baseProg *b,int alertType);
extern formula genericFormula[];
extern scenario genericScenario[];
extern int fS;
#define MAXKILLS 100
#define MAXLEVELS 10
int KILLS[MAXKILLS];
time_t KILLTIME[MAXKILLS];
int sendToMaster(char *buf,size_t l){
	logMessage(stderr,__FUNCTION__,__LINE__," %s %ld",buf,l);
	return(0);
}
extern char *MYHOSTNAME;
int lastIssuedAlert=0;
void freeBpMem(baseProg *b){
	b->violationsListN=0;;
	if (b->violationsList != 0){
		free(b->violationsList);
		b->violationsList = 0;
	}
}
baseProg BP[MAXBP];
void releaseBP(baseProg *b){
	freeBpMem(b);
	b->serverIP = 0;
	b->lastAction=0;
	b->lastIssuedAlert=0;
	b->violationsListN=0;;
	if (b->violationsList != 0){
		free(b->violationsList);
		b->violationsList = 0;
	}
}
void delBP(uint32_t serverIP,unsigned int serverPort){
	for(int i=0;i<MAXBP;++i){
		if ((BP[i].serverIP == serverIP) &&
			(BP[i].serverPort==serverPort)) {
			releaseBP(&BP[i]);
			return;
		}
	}
}
int RATETHRESHOLD=DEFAULTTHRESHOLD;
alertsRate *FIRST=0;
void issueAlert(baseProg *bp,char *name){
	fprintf(alertsOutFile,"%s.%d %ld %s %08x.%d",
		__FUNCTION__,__LINE__,time(0),
		name, bp->serverIP,bp->serverPort);
}
void issueClear(baseProg *bp){
	fprintf(alertsOutFile,"%s.%d %ld all clear on %08x.%d",
		__FUNCTION__,__LINE__,time(0),
		bp->serverIP,bp->serverPort);
}
void allClear(){
	int cnt=0;
	time_t now = time(0);
	for(int i=0;i<MAXBP;++i){
		if (BP[i].serverIP == 0){continue;}
		++cnt;
		if ((BP[i].lastIssuedAlert != 0) && 
			((now-BP[i].lastIssuedAlert)>AGETHRESHOLD)){
			double formulaScore[MAX_GENERIC_FORMULA];
			double scenarioScore[MAX_GENERIC_SCENARIO];
			size_t f=evaluateGenericScenarios(&BP[i],now,scenarioScore);
			size_t k=0;
			for(;k<f;++k){
				if (scenarioScore[k] >= 1){
					logMessage(stderr,__FUNCTION__,__LINE__," %d still true", i);
					break;
				}
			}
			if (k!=f){continue;}
			f=evaluateGenericFormulas(&BP[i],now,formulaScore);
			for(k=0;k<f;++k){
				if (formulaScore[k] >= 1){
					logMessage(stderr,__FUNCTION__,__LINE__," %s still true", genericFormula[i].name_);
					break;
				}
			}
			if (k!=f){continue;}
			logMessage(stderr,__FUNCTION__,__LINE__," all clear on %08x.%d",BP[i].serverIP,BP[i].serverPort);
			issueClear(&BP[i]);
			BP[i].lastIssuedAlert=0;
			delBP(BP[i].serverIP,BP[i].serverPort);
		}
	}
}
void initBP(){
	for(int i=0;i<MAXBP;++i){
		BP[i].serverIP=0;
		BP[i].violationsList=0;
		BP[i].violationsListN=0;
		BP[i].lastIssuedAlert=0;
		BP[i].ne=0;
		BP[i].nen=0;
		BP[i].nex=0;
	}
}
int getAppScore(char *appName,int alertType){
	alertsRate *f=FIRST;
	while (f != 0) {
		if (strncmp(f->appName,appName,LONGSTRING)==0){
			if (f->alertType==alertType){return(f->score);}
		}
		f = f->next;
	}
	return(-1);
}
#define UNKNOWNTHRESHOLD 10.0
double getAppThreshold(char *appName,int alertType){
	static int init=0;
	if (init == 0) {
		loadCurrentAR();
		init = 1;
	}
	int score=getAppScore(appName,alertType);
	if (score>=0){
		return((double)score);
	}
	switch(alertType){
	        case SLDTIMEOK:
                case SLDTIMEMAXLOW:
                case SLDTIMELOW:
                case SLDTIMEHIGH:
                case SLDTIMEMAXHIGH:
                case SLDTIMEZERO:
                case SLDCOUNTOK:
                case SLDCOUNTMAXLOW:
                case SLDCOUNTLOW:
                case SLDCOUNTHIGH:
                case SLDCOUNTMAXHIGH:
                case SLDCOUNTZERO:
                case SLDALERTANOMMAX:
		case PDDALIEN:
		case PDDANOM:
		case PDDSTARTSHORT:
		case PDDSTARTALIEN:
		case PDDSTARTANOM:
		case PDDOVERFLOW:
		case PRDZERO:
		case PRDALLMAX:
		case PRDALLSD2:
		case PRDMAX:
		case PRDSD2:
		case PRDALIEN:
		     return(RATETHRESHOLD);
		default:
			logMessage(stderr,__FUNCTION__,__LINE__,
				" ERROR unknown type %d",alertType);
	}
	logMessage(stderr,__FUNCTION__,__LINE__," ERROR unknown type %d",alertType);
	return(UNKNOWNTHRESHOLD);
}

void pruneBP(){
	time_t now = time(0);
	for(int i=0;i<MAXBP;++i){
		if (BP[i].serverIP != 0) {
			if ((BP[i].lastIssuedAlert==0) &&
				 ((now - BP[i].lastAction) > BPMAXAGE)){
				freeBpMem(&BP[i]);
				BP[i].serverIP = 0;
				BP[i].lastAction=0;
				BP[i].lastIssuedAlert=0;
				BP[i].violationsListN=0;
				if (BP[i].violationsList != 0){
					free(BP[i].violationsList);
					BP[i].violationsList=0;
				}
			}
		}
	}
}
baseProg* addBP(uint32_t serverIP,uint16_t serverPort,size_t level){
	int f=0;
	pruneBP();
	for (f=0;f<MAXBP;++f){
		if (BP[f].serverIP==0){
			 break;
		}
	}
	if (f==MAXBP){
		logMessage(stderr,__FUNCTION__,__LINE__," no room for BP %08x.%d",serverIP,serverPort);
		return(0);
	}
	BP[f].serverIP = serverIP;
	BP[f].serverPort = serverPort;
	BP[f].level = level;
	BP[f].scenarioSequenceX = 0;
	for(int i=0;i<AR_MAX_ALERT_TYPES;++i){
		char serverName[LONGSTRING];
		sprintf(serverName,"%08x.%d",BP[f].serverIP,BP[f].serverPort);
		BP[f].avg[i].value = 0.0;
		BP[f].avg[i].lastUpdate = 0.0;
		BP[f].max[i]=0;
		BP[f].threshold[i] = getAppThreshold(serverName,i);
	}
	BP[f].lastAction=time(0);
	BP[f].lastIssuedAlert=0;
	BP[f].violationsListN=0;
	BP[f].violationsList=0;
	return(&BP[f]);
}
int getBp(uint32_t serverIP,uint16_t serverPort,baseProg *bpList[]){
	int nBp=0;
	for(int i=0;i<MAXBP;++i){
		if ((BP[i].serverIP==serverIP) &&
			       	(BP[i].serverPort==serverPort)){
			bpList[nBp++]=&BP[i];
			BP[i].lastAction=time(0);
		}
	}
	if (nBp > 0){return(nBp);}
	if ((bpList[0]=addBP(serverIP,serverPort,0))==0){return(0);}
	return(1);
}
int updateRate(baseProg *b,int type,time_t start,double count){
	DqDecayAvg avg;
	avg.data = &b->avg[type];
	avg.decayFactor=HALFLIFE;
	decayAvgNewObservation(&avg,count,start);
	decayAvgNormalized(&avg,start);
	double t = b->avg[type].value;
	if (t > b->max[type]) {
		b->max[type] = t;
		if (RECORD==1){
			fprintf(alertsRateFile,"%08x.%d %d %d\n",
				b->serverIP,b->serverPort,type,(int)t);
			fflush(alertsRateFile);
		}
	}
	if (t > b->threshold[type]) {
		return(1);
	}
	return(0);
}
void updateAvg(DqDecayData current[],DqDecayData old[],double then,double now){
	for(int i=0;i<AR_MAX_ALERT_TYPES;++i){
		if (current[i].value < old[i].value){
			current[i].value = old[i].value;
			double diff = then-old[i].lastUpdate;
			if ((now - diff) > current[i].lastUpdate) {
				current[i].lastUpdate = (now-diff);
			}
		}
	}
}
int HCID=0;
#define SECFACTOR 10000000LL
#define USECFACTOR 10LL
#define USECPERSEC 1000000
int handleTrueScenario(int s,baseProg *b){
	size_t l=0;
	char buf[LONGSTRING];
	struct timeval nows;
	gettimeofday(&nows,0);
	long long now = (long long)nows.tv_sec * SECFACTOR
			+ (long long)nows.tv_usec * USECFACTOR;
	snprintf(buf,LONGSTRING,
		"det=HC.TRUE,id=%d,scenario=%s,cnt=0,begin=%lld,period=0,server=%08x.%d,action=KILL\n",
		HCID++,genericScenario[s].name_,now,b->serverIP,b->serverPort);
	logIR(buf,l=strlen(buf));
	sendToMaster(buf,l);
	return(0);
}
int handleTrueFormula(int f,baseProg *b){
	char buf[LONGSTRING];
	struct timeval nows;
	gettimeofday(&nows,0);
	long long now = (long long)nows.tv_sec * SECFACTOR
			 + (long long)nows.tv_usec * USECFACTOR;
	snprintf(buf,LONGSTRING,
		"det=HC.TRUE,id=%d,formula=%s,cnt=0,begin=%lld,period=0,server=%08x.%d,action=WOULDKILL\n",
		HCID++,genericFormula[f].name_,now,b->serverIP,b->serverPort);
	size_t l=0;
	logIR(buf,l=strlen(buf));
	sendToMaster(buf,l);
	return(0);
}
extern int addAlertToScenarioSequence(baseProg *b,int alertType,time_t alertTime);
void dumpCommon(alertCommon *c){
	logMessage(stderr,__FUNCTION__,__LINE__," %08x.%d,%d",
		c->serverIP, c->serverPort,c->type);
}
#define DFMT " %512d"
#define TFMT " %512lu"
void setAttackNames(char *a){
	size_t i=0;
	while (*a != '\0'){
		while (*a == ' '){++a;}
		while ((*a != ' ') && (*a != '\0')) {
			currentAttackName[currentAttackNameX][i++]=*a;
			++a;
		}
		currentAttackName[currentAttackNameX][i]='\0';
		i=0;
		size_t k=0;
		for (;k<currentAttackNameX;++k){
			if (strcmp(currentAttackName[currentAttackNameX],
						currentAttackName[k])==0){
				break;
			}
		}
		if (k<currentAttackNameX){
			strncpy(currentAttackName[currentAttackNameX],"END",LONGSTRING);
		       	continue;
		}
		if (currentAttackNameX >= MAXATTACKS){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"too many attacks");
		}
		++currentAttackNameX;
	}
}
int processSldDetail(uint32_t serverIp,unsigned int serverPort,int type,size_t count,time_t t){
	char *a=updateClassifierSld(serverIp,serverPort,type,count,t);
	if (a!=0){ setAttackNames(a); }
	return(0);
}
int processPddDetail(uint32_t serverIp,unsigned int serverPort,char *violation,size_t count,time_t t){
	char *a=updateClassifierPdd(serverIp,serverPort,violation,count,t);
	if (a!=0){ setAttackNames(a); }
	return(0);
}
int processPrdDetail(uint32_t serverIp,unsigned int serverPort,char *violation,char *d){
	time_t t=0;
	int cw1=0;
	int cw2=0;
	int count=0;
	char *bp=d;
	if (d==0){return(0);}
	if (*d=='\0'){return (0);}
	while (sscanf(bp,"(" TFMT "," DFMT "," DFMT "," DFMT ")",&t,&cw1,&cw2,&count)==4){ //NOLINT(cert-err34-c)
		char *a=updateClassifierPrd(serverIp,serverPort,violation,t,cw1,cw2,count);
		if (a!=0){ setAttackNames(a); }
		++bp;
		while (*bp != '('){
			if (*bp == '\0'){return(0);}
			++bp;
		}

	}
	logMessage(stderr,__FUNCTION__,__LINE__," cannot parse %s after %s",d,bp);
	return(-1);
}
int initCommonSLD(alertCommon *c, SLDALERT *a){
	c->start=0;
	c->impact=0;
	c->type=alertType2Int(a->violation);
	strncpy(c->violation,a->violation,sizeof(c->violation));
	c->alertTime = a->alertTime;
	c->clientIP=0;
	c->clientPort=0;
	c->serverIP=a->serverIP;
	c->serverPort=a->serverPort;
	c->count=a->count;
	c->start=a->alertTime;
	return(0);
}
int initCommonPRD(alertCommon *c, PRDALERT *a){
	c->start=0;
	c->impact=0;
	c->type=alertType2Int(a->violation);
	strncpy(c->violation,a->violation,sizeof(c->violation));
	c->alertTime = a->alertTime;
	c->clientIP=0;
	c->clientPort=0;
	c->serverIP=a->serverIP;
	c->serverPort=a->serverPort;
	c->count=a->count;
	c->start=a->start;
	return(0);
}
int initCommonPDD(alertCommon *c, PDDALERT *a){
	c->impact=a->impact;
	c->type=alertType2Int(a->violation);
	strncpy(c->violation,a->violation,sizeof(c->violation));
	c->start = a->start;
	c->alertTime = a->alertTime;
	c->clientIP=a->clientIP;
	c->clientPort=a->clientPort;
	c->serverIP=a->serverIP;
	c->serverPort=a->serverPort;
	c->count=a->count;
	return(0);
}
size_t evaluate(baseProg *b, alertCommon c,time_t now,int armed){
	size_t adas=0;
	double formulaScore[MAX_GENERIC_FORMULA];
	double scenarioScore[MAX_GENERIC_SCENARIO];
	int possibleScenarios=0;
	for(size_t k=0;k<c.count;++k){
		updateRate(b,c.type,(time_t)(c.start/USECPERSEC),(double)1);
	}
	possibleScenarios+=addAlertToScenarioSequence(b,c.type,now);
	switch(c.type){
	}
	if (possibleScenarios > 0){
		size_t f=evaluateGenericScenarios(b,(time_t)(c.start/USECPERSEC),scenarioScore);
		for(size_t k=0;k<f;++k){
			if (scenarioScore[k] >= 1){
				if (armed==1){
					issueAlert(b,genericScenario[k].name_);
					++adas;
				} else {
					logMessage(stderr,__FUNCTION__,__LINE__,
					"would issue ADA but not armed");
				}
			}
		}
	}
	size_t f=evaluateGenericFormulas(b,(time_t)(c.start/USECPERSEC),formulaScore);
	for(size_t k=0;k<f;++k){
		if (formulaScore[k] >= 1){
			if (armed==1){
				issueAlert(b,genericFormula[k].name_);
				++adas;
			} /*else {
#ifdef PUBLIC
				logMessage(stderr,__FUNCTION__,__LINE__,"
					"%s.%d would issue ADA but not armed");
#else
				pd3log_error("
					"%s.%d would issue ADA but not armed");
#endif
			}*/
		}
	}
	//	#ifdef PUBLIC
//	logMessage(stderr,__FUNCTION__,__LINE__,"
//	#else
//	pd3log_error("
//	#endif
	return(adas);
}

#define WTATZERO 116444736000000000
long long int windowsTime(time_t sec, time_t usec){
	long long int wt = (long long int) sec * SECFACTOR; //convert to hundreds of nano seconds
	wt += (long long int) usec * USECFACTOR; //convert to hundreds of nano seconds
	wt += WTATZERO;
	return(wt);
}
void addViolation(char *v,baseProg *l[],int n){
	int s=(int)strlen(v);
	for (int i=0;i<n;++i){
		if (l[i]->violationsList == 0){
			if ((l[i]->violationsList = malloc((s+MALLOCPAD)))==0){
				logMessage(stderr,__FUNCTION__,__LINE__," malloc failed");
				return;
			}
			l[i]->violationsListN=s+MALLOCPAD;
			strncpy(l[i]->violationsList,v,(size_t)s);
			l[i]->violationsList[s]='\0';
			continue;
		}
		if (strstr(l[i]->violationsList,v)==0) {
			int vl=(int)strlen(l[i]->violationsList);
			if ((vl+s+MALLOCPAD) > l[i]->violationsListN){
				if ((l[i]->violationsList = 
					realloc(l[i]->violationsList,(vl+s+MALLOCPAD)))
						==0){
					logMessage(stderr,__FUNCTION__,__LINE__," realloc failed");
					return;
				}
				l[i]->violationsListN=vl+s+MALLOCPAD;
			}
			strncat(&l[i]->violationsList[vl-1],",",s);
			strncat(&l[i]->violationsList[vl],v,(size_t)s);
		}
	}
}
#define VFMT " %80[^{},\n\t]"
char *extractSldDetail(char *d,uint32_t *clientIP,char *anom,size_t *count){
	if (*d == '\0'){return(0);}
	if (sscanf(d,"%08x" "," VFMT "," "%lu\n",clientIP,anom,count)!=3){ //NOLINT(cert-err34-c)
		logMessage(stderr,__FUNCTION__,__LINE__,"cannot parse %s",d);
		return(0);
	}
	while (*d != '\n'){
		if (*d == '\0'){return(0);}
	       	++d;
	}
	++d;
	return(d);
}
int processAlert(ALERT *a){
	int i=0;
	time_t now = time(0);
	baseProg *bpList[MAXBP];
	alertCommon c;
	size_t l=0;
	if (strcmp(a->message_name,"pddAlert")==0){
		if (initCommonPDD(&c,a->t.pddAlert)<0) {
			logMessage(stderr,__FUNCTION__,__LINE__," initCommon failed");
			return(-1);
		}
		processPddDetail(c.serverPort,c.serverPort,c.violation,c.count,c.start);
	}
	else if (strcmp(a->message_name,"prdAlert")==0){
		if (initCommonPRD(&c,a->t.prdAlert)<0) {
			logMessage(stderr,__FUNCTION__,__LINE__," initCommon failed");
			return(-1);
		}
		if (now-c.start >= 1){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"behind detector by %lu seconds",
					now-c.start);
		}
		processPrdDetail(c.serverPort,c.serverPort,c.violation,a->t.prdAlert->details);
	}
	else if (strcmp(a->message_name,"sldAlert")==0){
		if (initCommonSLD(&c,a->t.sldAlert)<0) {
			logMessage(stderr,__FUNCTION__,__LINE__," initCommon failed");
			return(-1);
		}
		if (now-c.start >= 1){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"behind detector by %lu seconds",
					now-c.start);
		}
		processSldDetail(c.serverPort,c.serverPort,c.type,c.count,c.start);
	} else {
		logMessage(stderr,__FUNCTION__,__LINE__," GOT NOT HANDLED %s alert",a->message_name);
		return(-1);
	}
	fprintf(alertsInFile,"%s.%d %ld %08x.%d %08x.%d %s.%d %d\n",
			__FUNCTION__,__LINE__,
		c.start,c.serverIP,c.serverPort,c.clientIP,c.clientPort,
		c.violation,c.type,c.impact);
	int nBp = getBp(c.serverIP,c.serverPort,bpList);
	if (nBp == 0){
		logMessage(stderr,__FUNCTION__,__LINE__," getBp", __FUNCTION__,__LINE__);
		return(-1);
	}
	// if the alert has a clientIP add it to network evidence.
	if (c.clientIP != 0){
		for (size_t i=0;i<nBp;++i){
			networkEvidence *ne=addNe(c.clientIP,c.type,c.count,&bpList[i]->ne,&bpList[i]->nex,&bpList[i]->nen);
			if (ne==0){
				logMessage(stderr,__FUNCTION__,__LINE__,
					"addNe failed ");
			} else{
				for(size_t a=0;a<currentAttackNameX;++a){
					attributeOneClient(currentAttackName[a],
							ne,now);
				}
			}
			outputGuilty(bpList[i]->ne,&bpList[i]->nex,now);
		}
	}
	//if this is an sld alert pull clients from details and add to ne
	if (strcmp(a->message_name,"sldAlert")==0){
		char *d=a->t.sldAlert->details;
		uint32_t clientIP=0;
		char anom[SHORTSTRING];
		size_t count=0;
		while ((d=extractSldDetail(d,&clientIP,anom,&count))!=0){
			for (size_t i=0;i<nBp;++i){
				networkEvidence *ne=addNe(clientIP,alertType2Int(anom),c.count,&bpList[i]->ne, &bpList[i]->nex,&bpList[i]->nen);
				if (ne==0){
					logMessage(stderr,__FUNCTION__,
						__LINE__,
						"addNe failed ");
				} else{
					for(size_t a=0;a<currentAttackNameX;++a){
						attributeOneClient(currentAttackName[a],
								ne,now);
					}
				}
				outputGuilty(bpList[i]->ne,&bpList[i]->nex,now);
			}
		}
	}
	/*for (size_t i=0;i<nBp;++i){
		for(size_t a=0;a<currentAttackNameX;++a){
			attributeClients(currentAttackName[a],bpList[i]->ne,bpList[i]->nex,now);
		}
	}*/
	/*if (ATTACKS != 0){
		for(size_t i=0;i<nBp;++i){
			for(size_t j=0;j<currentAttackNameX;++j){
#ifdef PUBLIC
				logMessage(stderr,__FUNCTION__,__LINE__,"%s",currentAttackName[j]);
#else
				pd3log_error("%s",currentAttackName[j]);
#endif
			}
		}
	}*/
	addViolation(c.violation,bpList,nBp);

//if issue an ADA at high level, do not issue ADAs at lower levels
	int armed=1;
	for (l=0;l<MAXLEVELS;++l){
		for(i=0;i<nBp;++i){
			if (bpList[i]->level==l){
				if (evaluate(bpList[i],c,now,armed)!=0){
//#ifdef PUBLIC
//					logMessage(stderr,__FUNCTION__,__LINE__," TRUE at %d",l);
//#else
//					pd3log_error(" TRUE at %d",l);
//#endif
					armed=0;
					++ATTACKS;
				}
			}
		}
	}
	return(0);
}
