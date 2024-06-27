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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "../common/flags.h"
#include "prdLog.h"
#include "defs.h"
#include "decay.h"
#include "classifier.h"
#define VFMT " %512[^{},\n\t()]"
char classRule[MAXCLASSRULES][512];
size_t classRuleX=0;
char *classifyAttack(classifier *b);
void addClassRule(char *r){
	if (classRuleX == MAXCLASSRULES) {
		pd3log_error("Too many class rules\n");
		return;
	}
	strcpy(classRule[classRuleX++],r);
}
void initClassifier(classifier *b){
	for(size_t i=0;i<MAXCW*2;++i){
			b->avgAllMax[i].value = 0.0;
			b->avgAllMax[i].lastUpdate = 0.0;
		for(size_t j=0;j<MAXCW*2;++j){
			b->avgMax[i][j].value = 0.0;
			b->avgMax[i][j].value = 0.0;
			b->avgMax[i][j].lastUpdate = 0.0;
			b->avgZero[i][j].lastUpdate = 0.0;
		}
	}
}
void updateRate(DqDecayData *b,time_t start,double count){
	DqDecayAvg avg;
	avg.data = b;
	avg.decayFactor=HALFLIFE;
	decayAvgNewObservation(&avg,count,start);
	decayAvgNormalized(&avg,start);
}
extern int AGETHRESHOLD;
double alertTypeAllMax2Age(classifier *b,int cw,time_t now){
	return((double)now-b->avgAllMax[cw].lastUpdate);
}
double alertTypeMax2Age(classifier *b,int cw1,int cw2,time_t now){
	return((double)now-b->avgMax[cw1][cw2].lastUpdate);
}
double alertTypeZero2Age(classifier *b,int cw1,int cw2,time_t now){
	return((double)now-b->avgZero[cw1][cw2].lastUpdate);
}
double alertTypeAllMax2Rate(classifier *b,int cw){
        DqDecayAvg avg;
        avg.data = &b->avgAllMax[cw];
        avg.decayFactor=HALFLIFE;
        decayAvgNormalized(&avg,time(0));
	return(b->avgAllMax[cw].value);
}
double alertTypeMax2Rate(classifier *b,int cw1,int cw2){
        DqDecayAvg avg;
        avg.data = &b->avgMax[cw1][cw2];
        avg.decayFactor=HALFLIFE;
        decayAvgNormalized(&avg,time(0));
	return(b->avgMax[cw1][cw2].value);
}
double alertTypeZero2Rate(classifier *b,int cw1,int cw2){
        DqDecayAvg avg;
        avg.data = &b->avgZero[cw1][cw2];
        avg.decayFactor=HALFLIFE;
        decayAvgNormalized(&avg,time(0));
	return(b->avgZero[cw1][cw2].value);
}
double alertType2AllMaxThreshold(classifier *b,int cw){
	return(b->thresholdAllMax[cw]);
}
double alertType2MaxThreshold(classifier *b,int cw1,int cw2){
	return(b->thresholdMax[cw1][cw2]);
}
double alertType2ZeroThreshold(classifier *b,int cw1,int cw2){
	return(b->thresholdZero[cw1][cw2]);
}
#define MINRATE .001
/*Attack signatures:
ackFlood: MAX(-SLAck/+SLAck) MAX(+SLRst/-SLSyn)
getFlood: ALLMAX(+SLAck) MAX(+SLAck/-SLAck) MAX(+SLAck/-SLSyn) MAX(+SLAck/+SLAckSyn)
stomp:    MAX(-SLAckPsh/-SLSyn) MAX(-SLAckPsh/+SLAckSyn) MAX(-SLAckPsh/+SLAckFin) MAX(+SLRst/-SLSyn)
synFlood: ALLMAX(+SLAckSyn) MAX(+SLAckSyn/-SLAckFin) MAX(+SLAckSyn/-SLAckPsh) MAX(+SLAckSyn/-SLAck)
*/
//ackFlood: MAX(-SLAck/+SLAck) MAX(+SLRst/-SLSyn)
bool ackFlood(classifier *b){
	static int init=0;
	static int InSLAck,OutSLAck,OutSLRst,InSLSyn;
	if (init==0){
		InSLAck=MAXCW-convertCwName("SLAck");
		OutSLAck=MAXCW+convertCwName("SLAck");
		OutSLRst=MAXCW+convertCwName("SLRst");
		InSLSyn=MAXCW-convertCwName("SLSyn");
		init=1;
	}
	if ((alertTypeMax2Rate(b,InSLAck,OutSLAck)>MINRATE) &&
		(alertTypeMax2Rate(b,OutSLRst,InSLSyn)>MINRATE)) {
		return(true);
	}
	return(false);
}
//synFlood: ALLMAX(+SLAckSyn) MAX(+SLAckSyn/-SLAckFin) MAX(+SLAckSyn/-SLAckPsh) MAX(+SLAckSyn/-SLAck)
bool synFlood(classifier *b){
	static int init=0;
	static int OutSLAckSyn,inSLAckFin,InSLAckPsh,InSLAck;
	if (init==0){
		OutSLAckSyn=MAXCW+convertCwName("SLAckSyn");
		inSLAckFin=MAXCW-convertCwName("SLAckFin");
		InSLAckPsh=MAXCW-convertCwName("SLAckPsh");
		InSLAck=MAXCW-convertCwName("SLAck");
		init=1;
	}
	if ((alertTypeAllMax2Rate(b,OutSLAckSyn)>MINRATE) &&
		(alertTypeMax2Rate(b,OutSLAckSyn,inSLAckFin)>MINRATE) &&
		(alertTypeMax2Rate(b,OutSLAckSyn,InSLAckPsh)>MINRATE) &&
		(alertTypeMax2Rate(b,OutSLAckSyn,InSLAck)>MINRATE)) {
		return(true);
	}
	return(false);
}
//getFlood: ALLMAX(+SLAck) MAX(+SLAck/-SLAck) MAX(+SLAck/-SLSyn) MAX(+SLAck/+SLAckSyn)
bool getFlood(classifier *b){
	static int init=0;
	static int OutSLAck,InSLAck,InSLSyn,OutSLAckSyn;
	if (init==0){
		OutSLAck=MAXCW+convertCwName("SLAck");
		InSLAck=MAXCW-convertCwName("SLAck");
		InSLSyn=MAXCW-convertCwName("SLSyn");
		OutSLAckSyn=MAXCW+convertCwName("SLAckSyn");
		init=1;
	}
	if ((alertTypeAllMax2Rate(b,OutSLAck)>MINRATE) &&
		(alertTypeMax2Rate(b,OutSLAck,InSLAck)>MINRATE) &&
		(alertTypeMax2Rate(b,OutSLAck,InSLSyn)>MINRATE) &&
		(alertTypeMax2Rate(b,OutSLAck,OutSLAckSyn)>MINRATE)) {
		return(true);
	}
	return(false);
}
//stomp:    MAX(-SLAckPsh/-SLSyn) MAX(-SLAckPsh/+SLAckSyn) MAX(-SLAckPsh/+SLAckFin) MAX(+SLRst/-SLSyn)
bool stomp(classifier *b){
	static int init=0;
	static int InSLAckPsh,InSLSyn,OutSLAckSyn,OutSLAckFin,OutSLRst;
	if (init==0){
		InSLAckPsh=MAXCW-convertCwName("SLAckPsh");
		InSLSyn=MAXCW-convertCwName("SLSyn");
		OutSLAckSyn=MAXCW+convertCwName("SLAckSyn");
		OutSLAckFin=MAXCW+convertCwName("SLAckFin");
		OutSLRst=MAXCW+convertCwName("SLRst");
		init=1;
	}
	if ((alertTypeMax2Rate(b,InSLAckPsh,InSLSyn)>MINRATE) &&
		(alertTypeMax2Rate(b,InSLAckPsh,OutSLAckSyn)>MINRATE) &&
		(alertTypeMax2Rate(b,InSLAckPsh,OutSLAckFin)>MINRATE) &&
		(alertTypeMax2Rate(b,OutSLRst,InSLSyn)>MINRATE)) {
		return(true);
	}
	return(false);
}
double evaluateClassTerm(classifier *b,char *r){
	if (strncmp(r,"ALLMAX(",7)==0){
		int cw;
		char cws[32];
		if (sscanf(&r[6],"(" VFMT ")",cws)!=1){
			pd3log_error("bad cw %s\n",r);
			return(0);
		}
		fprintf(stdout,"%s.%d %s\n",__FUNCTION__,__LINE__,cws);
		if(cws[0] == '-') cw=MAXCW-convertCwName(&cws[1]);
		else if(cws[0] == '+') cw=MAXCW+convertCwName(&cws[1]);
		else {
			pd3log_error("bad cw direction %s\n",r);
			return(0);
		}
		return(alertTypeAllMax2Rate(b,cw));
	}
	if (strncmp(r,"MAX(",4)==0){
		int cw1,cw2;
		char cws1[32],cws2[32];
		if (sscanf(&r[3],"(" VFMT "," VFMT ")",cws1,cws2)!=2){
			pd3log_error("bad cw %s\n",r);
			return(0);
		}
		fprintf(stdout,"%s.%d %s %s\n",__FUNCTION__,__LINE__,cws1,cws2);
		if(cws1[0] == '-') cw1=MAXCW-convertCwName(&cws1[1]);
		else if(cws1[0] == '+') cw1=MAXCW+convertCwName(&cws1[1]);
		else {
			pd3log_error("bad cw direction %s\n",r);
			return(0);
		}
		if(cws2[0] == '-') cw2=MAXCW-convertCwName(&cws2[1]);
		else if(cws2[0] == '+') cw2=MAXCW+convertCwName(&cws2[1]);
		else {
			pd3log_error("bad cw direction %s\n",r);
			return(0);
		}
		return(alertTypeMax2Rate(b,cw1,cw2));
	}
	if (strncmp(r,"ZERO(",5)==0){
		int cw1,cw2;
		char cws1[32],cws2[32];
		if (sscanf(&r[3],"(" VFMT "," VFMT ")",cws1,cws2)!=2){
			pd3log_error("bad cw %s\n",r);
			return(0);
		}
		if(cws1[0] == '-') cw1=MAXCW-convertCwName(&cws1[1]);
		else if(cws1[0] == '+') cw1=MAXCW+convertCwName(&cws1[1]);
		else {
			pd3log_error("bad cw direction %s\n", r);
			return(0);
		}
		if(cws2[0] == '-') cw2=MAXCW-convertCwName(&cws2[1]);
		else if(cws2[0] == '+') cw2=MAXCW+convertCwName(&cws2[1]);
		else {
			pd3log_error("bad cw direction %s\n",r);
			return(0);
		}
		return(alertTypeZero2Rate(b,cw1,cw2));
	}
	pd3log_error("NO SUCH FUNCTION: %s\n",r);
	return(0);
}

void evaluateClassRule(classifier *b,char *r,char *attackName){
	size_t n=strlen(r);
	size_t i=0;
	char attackClass[512];
	while(i<n){
		size_t j=0;
		while(r[i]==' ') ++i;
		while(r[i]!=' '){
			if(r[i] == '\0') return;
			attackClass[j++]=r[i++];
		}
		attackClass[j]='\0';
		break;
	}
	while(i<n){
		while(r[i]==' ') ++i;
		if (evaluateClassTerm(b,&r[i]) < MINRATE) return;
		while(r[i++]!=' '){
		      if(r[i] == '\0') break;
		}
	}
	strcat(attackName,attackClass);
}
		
char *classifyAttack(classifier *b){
	static char attackName[512];
	attackName[0]='\0';
	for(size_t i=0;i <classRuleX;++i){
		evaluateClassRule(b,classRule[i],attackName);
	}
	/*if (ackFlood(b)) strcat(attackName,"ACKFLOOD");
	if (getFlood(b)) strcat(attackName,"GETFLOOD");
	if (synFlood(b)) strcat(attackName,"SYNFLOOD");
	if (stomp(b)) strcat(attackName,"STOMP");*/
	if (attackName[0] == '\0'){
		strcat(attackName,"UNKNOWN");
	}
	return attackName;
}
void dumpClassifier(classifier *b){
	fprintf(stdout,"CLASSIFIER AT %ld\n",time(0));
	for(size_t i=0;i<2*MAXCW;++i){
		double t;
		char cws[32];
        	convertCw(i-MAXCW,cws,32);
		if ((t=alertTypeAllMax2Rate(b,i))!=0){
			fprintf(stdout,"CLASS ALLMAX[%s] = %g\n",cws,t);
		}
	}
	fprintf(stdout,"MAX:\n");
	for(size_t i=0;i<2*MAXCW;++i){
		for(size_t j=0;j<2*MAXCW;++j){
			double t;
			char cw1s[32],cw2s[32];
        		convertCw(i-MAXCW,cw1s,32);
        		convertCw(j-MAXCW,cw2s,32);
			if ((t=alertTypeMax2Rate(b,i,j))!=0){
				fprintf(stdout,"CLASS MAX[%ld/%ld %s/%s] = %g\n",
						i,j,cw1s,cw2s,t);
			}
		}
	}
	fprintf(stdout,"ZERO:\n");
	for(size_t i=0;i<2*MAXCW;++i){
		for(size_t j=0;j<2*MAXCW;++j){
			double t;
			char cw1s[32],cw2s[32];
        		convertCw(i-MAXCW,cw1s,32);
        		convertCw(j-MAXCW,cw2s,32);
			if ((t=alertTypeZero2Rate(b,i,j))!=0){
				fprintf(stdout,"CLASS ZERO[%ld/%ld %s/%s] = %g\n",
						i,j,cw1s,cw2s,t);
			}
		}
	}
}
