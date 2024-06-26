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
#include "../common/flags.h"
#include "../common/logMessage.h"
#include "alert.h"
#include "classifier.h"
#include "correlatorLog.h"
#include "defs.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#define MINRATE .01
#define VFMT " %512[^{},\n\t()]"
static FILE *classifierSsv=0;
char classRule[MAXCLASSRULES][LONGSTRING];
size_t classRuleX=0;
char *classifyAttack(serverClassifier *b,time_t t);
void dumpClassRules(){
	for (size_t i=0;i<classRuleX;++i){
		logMessage(stderr,__FUNCTION__,__LINE__,
			"classRule[%ld]: %s",i,classRule[i]);
	}
}
		
void addClassRule(char *r){
	if (classRuleX == MAXCLASSRULES) {
		logMessage(stderr,__FUNCTION__,__LINE__,
				"Too many class rules");
		return;
	}
	strncpy(classRule[classRuleX++],r,LONGSTRING);
}
serverClassifier SC[MAXSERVER];
serverClassifier *findClassifier(uint32_t serverIP,unsigned int serverPort){
	for(size_t i=0;i<MAXSERVER;++i){
		if ((SC[i].sIpAddr==serverIP) && (SC[i].sPort=serverPort)){
			return(&SC[i]);
		}
		if (SC[i].sIpAddr==0){
			SC[i].sIpAddr=serverIP;
			SC[i].sPort=serverPort;
			return(&SC[i]);
		}
	}
	return(0);
}
void initClassifier(){
	char *s=initCwMap();
	if (s != 0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"inCwMap failed: %s",s);
	}
	for(size_t i=0;i<MAXSERVER;++i){
		SC[i].sIpAddr=0;
		SC[i].sPort=0;
		initDecayData(&SC[i].avgPDDALIEN);
		initDecayData(&SC[i].avgPDDANOM);
		initDecayData(&SC[i].avgPDDSTARTSHORT);
		initDecayData(&SC[i].avgPDDSTARTALIEN);
		initDecayData(&SC[i].avgPDDSTARTANOM);
		initDecayData(&SC[i].avgSLD_TIME_OK);
        	initDecayData(&SC[i].avgSLD_TIME_MAXLOW);
        	initDecayData(&SC[i].avgSLD_TIME_MAXLOW);
        	initDecayData(&SC[i].avgSLD_TIME_LOW);
        	initDecayData(&SC[i].avgSLD_TIME_HIGH);
        	initDecayData(&SC[i].avgSLD_TIME_MAXHIGH);
        	initDecayData(&SC[i].avgSLD_TIME_ZERO);
        	initDecayData(&SC[i].avgSLD_COUNT_OK);
        	initDecayData(&SC[i].avgSLD_COUNT_MAXLOW);
        	initDecayData(&SC[i].avgSLD_COUNT_LOW);
        	initDecayData(&SC[i].avgSLD_COUNT_HIGH);
        	initDecayData(&SC[i].avgSLD_COUNT_MAXHIGH);
        	initDecayData(&SC[i].avgSLD_COUNT_ZERO);
        	initDecayData(&SC[i].avgSLD_ALERTANOMMAX);
		for(size_t j=0;j<MAXCW*2;++j){
			initDecayData(&SC[i].avgPRDALLMAX[j]);
			initDecayData(&SC[i].avgPRDALLSD2[j]);
			for(size_t k=0;k<MAXCW*2;++k){
				initDecayData(&SC[i].avgPRDZERO[j][k]);
				initDecayData(&SC[i].avgPRDSD2[j][k]);
				initDecayData(&SC[i].avgPRDMAX[j][k]);
			}
		}
	}
}
double updateClassifierPDDALIEN(serverClassifier *c,size_t count,time_t start){
	return(updateScore(&c->avgPDDALIEN,start,count));
}
double updateClassifierPDDANOM(serverClassifier *c,size_t count,time_t start){
	return(updateScore(&c->avgPDDANOM,start,count));
}
double updateClassifierPDDSTARTSHORT(serverClassifier *c,size_t count,time_t start){
	return(updateScore(&c->avgPDDSTARTSHORT,start,count));
}
double updateClassifierPDDSTARTLIEN(serverClassifier *c,size_t count,time_t start){
	return(updateScore(&c->avgPDDSTARTALIEN,start,count));
}
double updateClassifierPDDSTARTANOM(serverClassifier *c,size_t count,time_t start){
	return(updateScore(&c->avgPDDSTARTANOM,start,count));
}
double updateClassifierPRDALLMAX(serverClassifier *c, unsigned int cw,time_t start,int count){
	return(updateScore(&c->avgPRDALLMAX[cw+MAXCW],start,count));
}
double evaluateClassTerm(serverClassifier *b,char *r,time_t t){

	if (strncmp(r,"SLDCOUNTMAXHIGH",sizeof("SLDCOUNTMAXHIGH")-1)==0){
		return(getScore(&b->avgSLD_COUNT_MAXHIGH,t));
	}
	if (strncmp(r,"SLDCOUNTMAXLOW",sizeof("SLDCOUNTMAXLOW")-1)==0){
		return(getScore(&b->avgSLD_COUNT_MAXLOW,t));
	}
	if (strncmp(r,"SLDTIMEMAXHIGH",sizeof("SLDTIMEMAXHIGH")-1)==0){
		return(getScore(&b->avgSLD_TIME_MAXHIGH,t));
	}
	if (strncmp(r,"SLDTIMEMAXLOW",sizeof("SLDTIMEMAXLOW")-1)==0){
		return(getScore(&b->avgSLD_TIME_MAXLOW,t));
	}
	if (strncmp(r,"PDDSTARTANOM",sizeof("PDDSTARTANOM")-1)==0){
		return(getScore(&b->avgPDDSTARTANOM,t));
	}
	if (strncmp(r,"PDDSTARTALIEN",sizeof("PDDSTARTALIEN")-1)==0){
		return(getScore(&b->avgPDDSTARTALIEN,t));
	}
	if (strncmp(r,"PDDSTARTSHORT",sizeof("PDDSTARTSHORT")-1)==0){
		return(getScore(&b->avgPDDSTARTSHORT,t));
	}
	if (strncmp(r,"SLDCOUNTZERO",sizeof("SLDCOUNTZERO")-1)==0){
		return(getScore(&b->avgSLD_COUNT_ZERO,t));
	}
	if (strncmp(r,"SLDCOUNTHIGH",sizeof("SLDCOUNTHIGH")-1)==0){
		return(getScore(&b->avgSLD_COUNT_HIGH,t));
	}
	if (strncmp(r,"SLDCOUNTLOW",sizeof("SLDCOUNTLOW")-1)==0){
		return(getScore(&b->avgSLD_COUNT_LOW,t));
	}
	if (strncmp(r,"SLDTIMEHIGH",sizeof("SLDTIMEHIGH")-1)==0){
		return(getScore(&b->avgSLD_TIME_HIGH,t));
	}
	if (strncmp(r,"SLDTIMEZERO",sizeof("SLDTIMEZERO")-1)==0){
		return(getScore(&b->avgSLD_TIME_ZERO,t));
	}
	if (strncmp(r,"SLDTIMELOW",sizeof("SLDTIMELOW")-1)==0){
		return(getScore(&b->avgSLD_TIME_LOW,t));
	}
	if (strncmp(r,"SLDCOUNTOK",sizeof("SLDCOUNTOK")-1)==0){
		return(getScore(&b->avgSLD_COUNT_OK,t));
	}
	if (strncmp(r,"SLDTIMEOK",sizeof("SLDTIMEOK")-1)==0){
		return(getScore(&b->avgSLD_TIME_OK,t));
	}
	if (strncmp(r,"PDDALIEN",sizeof("PDDALIEN")-1)==0){
		return(getScore(&b->avgPDDALIEN,t));
	}
	if (strncmp(r,"PDDANOM",sizeof("PDDANOM")-1)==0){
		return(getScore(&b->avgPDDANOM,t));
	}
	if (strncmp(r,"PRDALLMAX(",sizeof("PRDALLMAX(")-1)==0){
		int cw=0;
		char cws[SHORTSTRING];
		if (sscanf(&r[sizeof("PRDALLMAX")-1],"(" VFMT ")",cws)!=1){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"bad cw %s",r);
			return(0);
		}
		if(cws[0] == '-'){cw=MAXCW-convertCwName(&cws[1]);}
		else if(cws[0] == '+'){cw=MAXCW+convertCwName(&cws[1]);}
		else {
			logMessage(stderr,__FUNCTION__,__LINE__,
					"bad cw direction %s",r);
			return(0);
		}
		return(getScore(&b->avgPRDALLMAX[cw],t));
	}
	if (strncmp(r,"PRDALLSD2(",sizeof("PRDALLSD2(")-1)==0){
		int cw=0;
		char cws[SHORTSTRING];
		if (sscanf(&r[sizeof("PRDALLSD2")-1],"(" VFMT ")",cws)!=1){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"bad cw %s",r);
			return(0);
		}
		if(cws[0] == '-'){cw=MAXCW-convertCwName(&cws[1]);}
		else if(cws[0] == '+'){cw=MAXCW+convertCwName(&cws[1]);}
		else {
			logMessage(stderr,__FUNCTION__,__LINE__,
					"bad cw direction %s",r);
			return(0);
		}
		return(getScore(&b->avgPRDALLSD2[cw],t));
	}
	if (strncmp(r,"PRDMAX(",sizeof("PRDMAX(")-1)==0){
		int cw1=0;
		int cw2=0;
		char cws1[SHORTSTRING];
		char cws2[SHORTSTRING];
		if (sscanf(&r[sizeof("PRDMAX")-1],"(" VFMT "," VFMT ")",cws1,cws2)!=2){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"bad cw %s",r);
			return(0);
		}
		if(cws1[0] == '-'){cw1=MAXCW-convertCwName(&cws1[1]);}
		else if(cws1[0] == '+'){cw1=MAXCW+convertCwName(&cws1[1]);}
		else {
			logMessage(stderr,__FUNCTION__,__LINE__,
					"bad cw direction %s",r);
			return(0);
		}
		if(cws2[0] == '-'){cw2=MAXCW-convertCwName(&cws2[1]);}
		else if(cws2[0] == '+'){cw2=MAXCW+convertCwName(&cws2[1]);}
		else {
			logMessage(stderr,__FUNCTION__,__LINE__,
					"bad cw direction %s",r);
			return(0);
		}
		return(getScore(&b->avgPRDMAX[cw1][cw2],t));
	}
	if (strncmp(r,"PRDSD2(",sizeof("PRDSD2(")-1)==0){
		int cw1=0;
		int cw2=0;
		char cws1[SHORTSTRING];
		char cws2[SHORTSTRING];
		if (sscanf(&r[sizeof("PRDSD2")-1],"(" VFMT "," VFMT ")",cws1,cws2)!=2){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"bad cw %s",r);
			return(0);
		}
		if(cws1[0] == '-'){cw1=MAXCW-convertCwName(&cws1[1]);}
		else if(cws1[0] == '+'){cw1=MAXCW+convertCwName(&cws1[1]);}
		else {
			logMessage(stderr,__FUNCTION__,__LINE__,
					"bad cw direction %s",r);
			return(0);
		}
		if(cws2[0] == '-'){cw2=MAXCW-convertCwName(&cws2[1]);}
		else if(cws2[0] == '+'){cw2=MAXCW+convertCwName(&cws2[1]);}
		else {
			logMessage(stderr,__FUNCTION__,__LINE__,
					"bad cw direction %s",r);
			return(0);
		}
		return(getScore(&b->avgPRDSD2[cw1][cw2],t));
	}
	if (strncmp(r,"PRDZERO(",sizeof("PRDZERO(")-1)==0){
		int cw1=0;
		int cw2=0;
		char cws1[SHORTSTRING];
		char cws2[SHORTSTRING];
		if (sscanf(&r[sizeof("PRDZERO")-1],"(" VFMT "," VFMT ")",cws1,cws2)!=2){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"bad cw %s",r);
			return(0);
		}
		if(cws1[0] == '-'){cw1=MAXCW-convertCwName(&cws1[1]);}
		else if(cws1[0] == '+'){cw1=MAXCW+convertCwName(&cws1[1]);}
		else {
			logMessage(stderr,__FUNCTION__,__LINE__,
					"bad cw direction %s", r);
			return(0);
		}
		if(cws2[0] == '-'){cw2=MAXCW-convertCwName(&cws2[1]);}
		else if(cws2[0] == '+'){cw2=MAXCW+convertCwName(&cws2[1]);}
		else {
			logMessage(stderr,__FUNCTION__,__LINE__,
					"bad cw direction %s",r);
			return(0);
		}
		return(getScore(&b->avgPRDZERO[cw1][cw2],t));
	}
	logMessage(stderr,__FUNCTION__,__LINE__,
			"NO SUCH FUNCTION: %s",r);
	return(0);
}
char *termString(const char *t){
	static char s[LONGSTRING];
	size_t i=0;
	while((s[i]=t[i])!='\0'){
		if (s[i]==' '){
			s[i]='\0';
			return(s);
		}
		++i;
	}
	return(s);
}
void evaluateClassRule(serverClassifier *b,char *r,char *attackName,time_t t,size_t *size){
	size_t n=strlen(r);
	size_t i=0;
	size_t nTerms=0;;
	char attackClass[LONGSTRING];
	while(i<n){
		size_t j=0;
		while(r[i]==' '){++i;}
		while(r[i]!=' '){
			if(r[i] == '\0'){return;}
			attackClass[j++]=r[i++];
		}
		attackClass[j]='\0';
		break;
	}
	while(i<n){
		while(r[i]==' '){++i;}
		if (evaluateClassTerm(b,&r[i],t) < MINRATE){
		       	return;
		}
		++nTerms;
		while(r[i++]!=' '){
		      if(r[i] == '\0'){break;}
		}
	}
	if (nTerms<*size){return;}
	if (nTerms==*size){
		strncat(attackName," ",
				LONGSTRING-strlen(attackName));
		strncat(attackName,attackClass,
				LONGSTRING-strlen(attackName));
	} else {
		strncpy(attackName,attackClass,LONGSTRING);
	}
	*size=nTerms;
}
void dumpClassifierPDDALIEN(FILE *f,serverClassifier *c,time_t t){
	if (c->avgPDDALIEN.value<MINRATE){return;}
	getScore(&c->avgPDDALIEN,t);
	if (c->avgPDDALIEN.value<MINRATE) {return;}
	fprintf(f,"%lu PDDALIEN|%g\n",t,c->avgPDDALIEN.value);
}
void dumpClassifierPDDANOM(FILE *f,serverClassifier *c,time_t t){
	if (c->avgPDDANOM.value<MINRATE){return;}
	getScore(&c->avgPDDANOM,t);
	if (c->avgPDDANOM.value<MINRATE) {return;}
	fprintf(f,"%lu PDDANOM|%g\n",t,c->avgPDDANOM.value);
}
void dumpClassifierPDDSTARTSHORT(FILE *f,serverClassifier *c,time_t t){
	if (c->avgPDDSTARTSHORT.value<MINRATE){return;}
	getScore(&c->avgPDDSTARTSHORT,t);
	if (c->avgPDDSTARTSHORT.value<MINRATE) {return;}
	fprintf(f,"%lu PDDSTARTSHORT|%g\n",t,c->avgPDDSTARTSHORT.value);
}
void dumpClassifierPDDSTARTALIEN(FILE *f,serverClassifier *c,time_t t){
	if (c->avgPDDSTARTALIEN.value<MINRATE){return;}
	getScore(&c->avgPDDSTARTALIEN,t);
	if (c->avgPDDSTARTALIEN.value<MINRATE) {return;}
	fprintf(f,"%lu PDDSTARTALIEN|%g\n",t,c->avgPDDSTARTALIEN.value);
}
void dumpClassifierPDDSTARTANOM(FILE *f,serverClassifier *c,time_t t){
	if (c->avgPDDSTARTANOM.value<MINRATE){return;}
	getScore(&c->avgPDDSTARTANOM,t);
	if (c->avgPDDSTARTANOM.value<MINRATE) {return;}
	fprintf(f,"%lu PDDSTARTANOM|%g\n",t,c->avgPDDSTARTANOM.value);
}
void dumpClassifierSLDTIMEOK(FILE *f,serverClassifier *c,time_t t){
	if (c->avgSLD_TIME_OK.value<MINRATE){return;}
	getScore(&c->avgSLD_TIME_OK,t);
	if (c->avgSLD_TIME_OK.value<MINRATE) {return;}
	fprintf(f,"%lu SLDTIMEOK|%g\n",t,c->avgSLD_TIME_OK.value);
}
void dumpClassifierSLDTIMEMAXLOW(FILE *f,serverClassifier *c,time_t t){
	if (c->avgSLD_TIME_MAXLOW.value<MINRATE){return;}
	getScore(&c->avgSLD_TIME_MAXLOW,t);
	if (c->avgSLD_TIME_MAXLOW.value<MINRATE) {return;}
	fprintf(f,"%lu SLDTIMEMAXLOW|%g\n",t,c->avgSLD_TIME_MAXLOW.value);
}
void dumpClassifierSLDTIMELOW(FILE *f,serverClassifier *c,time_t t){
	if (c->avgSLD_TIME_LOW.value<MINRATE){return;}
	getScore(&c->avgSLD_TIME_LOW,t);
	if (c->avgSLD_TIME_LOW.value<MINRATE) {return;}
	fprintf(f,"%lu SLDTIMELOW|%g\n",t,c->avgSLD_TIME_LOW.value);
}
void dumpClassifierSLDTIMEHIGH(FILE *f,serverClassifier *c,time_t t){
	if (c->avgSLD_TIME_HIGH.value<MINRATE){return;}
	getScore(&c->avgSLD_TIME_HIGH,t);
	if (c->avgSLD_TIME_HIGH.value<MINRATE) {return;}
	fprintf(f,"%lu SLDTIMEHIGH|%g\n",t,c->avgSLD_TIME_HIGH.value);
}
void dumpClassifierSLDTIMEMAXHIGH(FILE *f,serverClassifier *c,time_t t){
	if (c->avgSLD_TIME_MAXHIGH.value<MINRATE){return;}
	getScore(&c->avgSLD_TIME_MAXHIGH,t);
	if (c->avgSLD_TIME_MAXHIGH.value<MINRATE) {return;}
	fprintf(f,"%lu SLDTIMEMAXHIGH|%g\n",t,c->avgSLD_TIME_MAXHIGH.value);
}
void dumpClassifierSLDTIMEZERO(FILE *f,serverClassifier *c,time_t t){
	if (c->avgSLD_TIME_ZERO.value<MINRATE){return;}
	getScore(&c->avgSLD_TIME_ZERO,t);
	if (c->avgSLD_TIME_ZERO.value<MINRATE) {return;}
	fprintf(f,"%lu SLDTIMEZERO|%g\n",t,c->avgSLD_TIME_ZERO.value);
}
void dumpClassifierSLDCOUNTOK(FILE *f,serverClassifier *c,time_t t){
	if (c->avgSLD_COUNT_OK.value<MINRATE){return;}
	getScore(&c->avgSLD_COUNT_OK,t);
	if (c->avgSLD_COUNT_OK.value<MINRATE) {return;}
	fprintf(f,"%lu SLDCOUNTOK|%g\n",t,c->avgSLD_COUNT_OK.value);
}
void dumpClassifierSLDCOUNTMAXLOW(FILE *f,serverClassifier *c,time_t t){
	if (c->avgSLD_COUNT_MAXLOW.value<MINRATE){return;}
	getScore(&c->avgSLD_COUNT_MAXLOW,t);
	if (c->avgSLD_COUNT_MAXLOW.value<MINRATE) {return;}
	fprintf(f,"%lu SLDCOUNTMAXLOW|%g\n",t,c->avgSLD_COUNT_MAXLOW.value);
}
void dumpClassifierSLDCOUNTLOW(FILE *f,serverClassifier *c,time_t t){
	if (c->avgSLD_COUNT_LOW.value<MINRATE){return;}
	getScore(&c->avgSLD_COUNT_LOW,t);
	if (c->avgSLD_COUNT_LOW.value<MINRATE) {return;}
	fprintf(f,"%lu SLDCOUNTLOW|%g\n",t,c->avgSLD_COUNT_LOW.value);
}
void dumpClassifierSLDCOUNTHIGH(FILE *f,serverClassifier *c,time_t t){
	if (c->avgSLD_COUNT_HIGH.value<MINRATE){return;}
	getScore(&c->avgSLD_COUNT_HIGH,t);
	if (c->avgSLD_COUNT_HIGH.value<MINRATE) {return;}
	fprintf(f,"%lu SLDCOUNTHIGH|%g\n",t,c->avgSLD_COUNT_HIGH.value);
}
void dumpClassifierSLDCOUNTMAXHIGH(FILE *f,serverClassifier *c,time_t t){
	if (c->avgSLD_COUNT_MAXHIGH.value<MINRATE){return;}
	getScore(&c->avgSLD_COUNT_MAXHIGH,t);
	if (c->avgSLD_COUNT_MAXHIGH.value<MINRATE) {return;}
	fprintf(f,"%lu SLDCOUNTMAXHIGH|%g\n",t,c->avgSLD_COUNT_MAXHIGH.value);
}
void dumpClassifierSLDCOUNTZERO(FILE *f,serverClassifier *c,time_t t){
	if (c->avgSLD_COUNT_ZERO.value<MINRATE){return;}
	getScore(&c->avgSLD_COUNT_ZERO,t);
	if (c->avgSLD_COUNT_ZERO.value<MINRATE) {return;}
	fprintf(f,"%lu SLDCOUNTZERO|%g\n",t,c->avgSLD_COUNT_ZERO.value);
}
void dumpClassifierSLDANOMMAX(FILE *f,serverClassifier *c,time_t t){
	if (c->avgSLD_ALERTANOMMAX.value<MINRATE){return;}
	getScore(&c->avgSLD_ALERTANOMMAX,t);
	if (c->avgSLD_ALERTANOMMAX.value<MINRATE) {return;}
	fprintf(f,"%lu SLDALERTANOMMAX|%g\n",t,c->avgSLD_ALERTANOMMAX.value);
}
void dumpClassifierPRDALLMAX(FILE *f,serverClassifier *c,time_t t){
	for(int i=0;i<MAXCW*2;++i){
		if (c->avgPRDALLMAX[i].value<MINRATE) {continue;}
		getScore(&c->avgPRDALLMAX[i],t);
		if (c->avgPRDALLMAX[i].value<MINRATE) {continue;}
		char cw[SHORTSTRING];
		convertCw(i-MAXCW,cw,SHORTSTRING);
		fprintf(f,"%lu PRDALLMAX|%s %g\n",t,cw,c->avgPRDALLMAX[i].value);
	}
}
double updateClassifierPRDALLSD2(serverClassifier *c, unsigned int cw,time_t start,int count){
	return(updateScore(&c->avgPRDALLSD2[cw+MAXCW],start,count));
}
void dumpClassifierPRDALLSD2(FILE *f,serverClassifier *c,time_t t){
	for(int i=0;i<MAXCW*2;++i){
		if (c->avgPRDALLSD2[i].value<MINRATE) {continue;}
		getScore(&c->avgPRDALLSD2[i],t);;
		if (c->avgPRDALLSD2[i].value<MINRATE) {continue;}
		char cw[SHORTSTRING];
		convertCw(i-MAXCW,cw,SHORTSTRING);
		fprintf(f,"%lu PRDALLSD2|%s %g\n",t,cw,c->avgPRDALLSD2[i].value);
	}
}
double updateClassifierPRDZERO(serverClassifier *c, unsigned int cw1,unsigned int cw2,time_t start,int count){
	return(updateScore(&c->avgPRDZERO[cw1+MAXCW][cw2+MAXCW],start,count));
}
void dumpClassifierPRDZERO(FILE *f,serverClassifier *c,time_t t){
	for(int i=0;i<MAXCW*2;++i){
		for(int j=0;j<MAXCW*2;++j){
			if (c->avgPRDZERO[i][j].value<MINRATE) {continue;}
			getScore(&c->avgPRDZERO[i][j],t);
			if (c->avgPRDZERO[i][j].value<MINRATE) {continue;}
			char cw1[SHORTSTRING];
			char cw2[SHORTSTRING];
			convertCw(i-MAXCW,cw1,SHORTSTRING);
			convertCw(j-MAXCW,cw2,SHORTSTRING);
			fprintf(f,"%lu PRDZERO|%s|%s %g\n",t,cw1,cw2,c->avgPRDZERO[i][j].value);
		}
	}
}
double updateClassifierPRDSD2(serverClassifier *c, unsigned int cw1,unsigned int cw2,time_t start,int count){
	return(updateScore(&c->avgPRDSD2[cw1+MAXCW][cw2+MAXCW],start,count));
}
void dumpClassifierPRDSD2(FILE *f,serverClassifier *c,time_t t){
	for(int i=0;i<MAXCW*2;++i){
		for(int j=0;j<MAXCW*2;++j){
			if (c->avgPRDSD2[i][j].value<MINRATE) {continue;}
			getScore(&c->avgPRDSD2[i][j],t);
			if (c->avgPRDSD2[i][j].value<MINRATE) {continue;}
			char cw1[SHORTSTRING];
			char cw2[SHORTSTRING];
			convertCw(i-MAXCW,cw1,SHORTSTRING);
			convertCw(j-MAXCW,cw2,SHORTSTRING);
			fprintf(f,"%lu PRDSD2|%s|%s %g\n",t,cw1,cw2,c->avgPRDSD2[i][j].value);
		}
	}
}
double updateClassifierPRDMAX(serverClassifier *c, unsigned int cw1,unsigned int cw2,time_t start,int count){
	return(updateScore(&c->avgPRDMAX[cw1+MAXCW][cw2+MAXCW],start,count));
}
double updateClassifierSLDTIMEOK(serverClassifier *c,time_t start,int count){
	return(updateScore(&c->avgSLD_TIME_OK,start,count));
}
double updateClassifierSLDTIMEMAXLOW(serverClassifier *c,time_t start,int count){
	return(updateScore(&c->avgSLD_TIME_MAXLOW,start,count));
}
double updateClassifierSLDTIMELOW(serverClassifier *c,time_t start,int count){
	return(updateScore(&c->avgSLD_TIME_LOW,start,count));
}
double updateClassifierSLDTIMEHIGH(serverClassifier *c,time_t start,int count){
	return(updateScore(&c->avgSLD_TIME_HIGH,start,count));
}
double updateClassifierSLDTIMEMAXHIGH(serverClassifier *c,time_t start,int count){
	return(updateScore(&c->avgSLD_TIME_MAXHIGH,start,count));
}
double updateClassifierSLDTIMEZERO(serverClassifier *c,time_t start,int count){
	return(updateScore(&c->avgSLD_TIME_ZERO,start,count));
}
double updateClassifierSLDCOUNTOK(serverClassifier *c,time_t start,int count){
	return(updateScore(&c->avgSLD_COUNT_OK,start,count));
}
double updateClassifierSLDCOUNTMAXLOW(serverClassifier *c,time_t start,int count){
	return(updateScore(&c->avgSLD_COUNT_MAXLOW,start,count));
}
double updateClassifierSLDCOUNTLOW(serverClassifier *c,time_t start,int count){
	return(updateScore(&c->avgSLD_COUNT_LOW,start,count));
}
double updateClassifierSLDCOUNTHIGH(serverClassifier *c,time_t start,int count){
	return(updateScore(&c->avgSLD_COUNT_HIGH,start,count));
}
double updateClassifierSLDCOUNTMAXHIGH(serverClassifier *c,time_t start,int count){
	return(updateScore(&c->avgSLD_COUNT_MAXHIGH,start,count));
}
double updateClassifierSLDCOUNTZERO(serverClassifier *c,time_t start,int count){
	return(updateScore(&c->avgSLD_COUNT_ZERO,start,count));
}
double updateClassifierSLDALERTANOMMAX(serverClassifier *c,time_t start,int count){
	return(updateScore(&c->avgSLD_ALERTANOMMAX,start,count));
}
void dumpClassifierPRDMAX(FILE *f,serverClassifier *c,time_t t){
	for(int i=0;i<MAXCW*2;++i){
		for(int j=0;j<MAXCW*2;++j){
			if (c->avgPRDMAX[i][j].value<MINRATE) {continue;}
			getScore(&c->avgPRDMAX[i][j],t);
			if (c->avgPRDMAX[i][j].value<MINRATE) {continue;}
			char cw1[SHORTSTRING];
			char cw2[SHORTSTRING];
			convertCw(i-MAXCW,cw1,SHORTSTRING);
			convertCw(j-MAXCW,cw2,SHORTSTRING);
			fprintf(f,"%lu PRDMAX|%s|%s %g\n",t,cw1,cw2,c->avgPRDMAX[i][j].value);
		}
	}
}
char *classifyAttack(serverClassifier *b,time_t t){
	static size_t attackSize=0;
	static char attackName[LONGSTRING];
	attackName[0]='\0';
	for(size_t i=0;i <classRuleX;++i){
		evaluateClassRule(b,classRule[i],attackName,t,&attackSize);
	}
	if (attackName[0] == '\0'){attackSize=0; return(0); }
	return attackName;
}
static time_t lastClassifyTime=0;
void initClassifierCsv(){
	if (classifierSsv!=0){return;}
	if ((classifierSsv=fopen("/tmp/classifier.csv","w+e"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
			"fopen(/tmp/classifier.csv): %s",
				strerror(errno));
	}
}
void dumpClassifier(FILE *classifierSsv,serverClassifier *sc,time_t now){
	dumpClassifierSLDTIMEOK(classifierSsv,sc,now);
	dumpClassifierSLDTIMEMAXLOW(classifierSsv,sc,now);
	dumpClassifierSLDTIMELOW(classifierSsv,sc,now);
	dumpClassifierSLDTIMEHIGH(classifierSsv,sc,now);
	dumpClassifierSLDTIMEMAXHIGH(classifierSsv,sc,now);
	dumpClassifierSLDTIMEZERO(classifierSsv,sc,now);
	dumpClassifierSLDCOUNTOK(classifierSsv,sc,now);
	dumpClassifierSLDCOUNTMAXLOW(classifierSsv,sc,now);
	dumpClassifierSLDCOUNTLOW(classifierSsv,sc,now);
	dumpClassifierSLDCOUNTHIGH(classifierSsv,sc,now);
	dumpClassifierSLDCOUNTMAXHIGH(classifierSsv,sc,now);
	dumpClassifierSLDCOUNTZERO(classifierSsv,sc,now);
	dumpClassifierSLDANOMMAX(classifierSsv,sc,now);
	dumpClassifierPDDALIEN(classifierSsv,sc,now);
	dumpClassifierPDDANOM(classifierSsv,sc,now);
	dumpClassifierPDDSTARTSHORT(classifierSsv,sc,now);
	dumpClassifierPDDSTARTALIEN(classifierSsv,sc,now);
	dumpClassifierPDDSTARTANOM(classifierSsv,sc,now);
	dumpClassifierPRDALLMAX(classifierSsv,sc,now);
	dumpClassifierPRDALLSD2(classifierSsv,sc,now);
	dumpClassifierPRDZERO(classifierSsv,sc,now);
	dumpClassifierPRDMAX(classifierSsv,sc,now);
	dumpClassifierPRDSD2(classifierSsv,sc,now);
	fflush(classifierSsv);
}
char *updateClassifierSld(uint32_t serverIP,unsigned int serverPort,int type,size_t count,time_t t){
	time_t now=time(0);
	initClassifierCsv();
	serverClassifier *sc=findClassifier(serverIP,serverPort);
	if (sc == 0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"no classifier for %08x.%u", serverIP,serverPort);
		return 0;
	}
	switch(type){
	case SLDTIMEOK:
		updateClassifierSLDTIMEOK(sc,t,count); break;
        case SLDTIMEMAXLOW:
		updateClassifierSLDTIMEMAXLOW(sc,t,count); break;
        case SLDTIMELOW:
		updateClassifierSLDTIMELOW(sc,t,count); break;
        case SLDTIMEHIGH:
		updateClassifierSLDTIMEHIGH(sc,t,count); break;
        case SLDTIMEMAXHIGH:
		updateClassifierSLDTIMEMAXHIGH(sc,t,count); break;
        case SLDTIMEZERO:
		updateClassifierSLDTIMEZERO(sc,t,count); break;
        case SLDCOUNTOK:
		updateClassifierSLDCOUNTOK(sc,t,count); break;
        case SLDCOUNTMAXLOW:
		updateClassifierSLDCOUNTMAXLOW(sc,t,count); break;
        case SLDCOUNTLOW:
		updateClassifierSLDCOUNTLOW(sc,t,count); break;
        case SLDCOUNTHIGH:
		updateClassifierSLDCOUNTHIGH(sc,t,count); break;
        case SLDCOUNTMAXHIGH:
		updateClassifierSLDCOUNTMAXHIGH(sc,t,count); break;
        case SLDCOUNTZERO:
		updateClassifierSLDCOUNTZERO(sc,t,count); break;
	case SLDALERTANOMMAX:
		updateClassifierSLDALERTANOMMAX(sc,t,count); break;
	default:
		logMessage(stderr,__FUNCTION__,__LINE__,
				"unknown type %d",type);
	}
	if ((now-lastClassifyTime)<1){return 0;}
	lastClassifyTime=now;
	char *a=classifyAttack(sc,t);
	if (a != 0){
		++ATTACKS;
		logMessage(stderr,__FUNCTION__,__LINE__,
				"attack: %s",a);
	}
	dumpClassifier(classifierSsv,sc,now);
	return(a);
}
char *updateClassifierPdd(uint32_t serverIP,unsigned int serverPort,char *violation,size_t count,time_t t){
	time_t now=time(0);
	initClassifierCsv();
	serverClassifier *sc=findClassifier(serverIP,serverPort);
	if (sc == 0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"no classifier for %08x.%u", serverIP,serverPort);
		return 0;
	}
	if(strcmp(violation,"PDDALIEN")==0){
		updateClassifierPDDALIEN(sc,count,t);
	}
	else if(strcmp(violation,"PDDANOM")==0){
		updateClassifierPDDANOM(sc,count,t);
	}
	else if(strcmp(violation,"PDDSTARTSHORT")==0){
		updateClassifierPDDSTARTSHORT(sc,count,t);
	}
	else if(strcmp(violation,"PDDSTARTALIEN")==0){
		updateClassifierPDDSTARTLIEN(sc,count,t);
	}
	else if(strcmp(violation,"PDDSTARTANOM")==0){
		updateClassifierPDDSTARTANOM(sc,count,t);
	} else {
		logMessage(stderr,__FUNCTION__,__LINE__,
			"UNKNOWN PDD ALERT %s",violation);
		return 0;
	}
	if ((now-lastClassifyTime)<1){return 0;}
	lastClassifyTime=now;
	char *a=classifyAttack(sc,t);
	if (a != 0){
		++ATTACKS;
		logMessage(stderr,__FUNCTION__,__LINE__,
				"attack: %s",a);
	}
	dumpClassifier(classifierSsv,sc,now);
	return(a);
}
char *updateClassifierPrd(uint32_t serverIP,unsigned int serverPort,char *violation,time_t t,int cw1,int cw2,int count){
	initClassifierCsv();
	time_t now=time(0);
	char cws1[SHORTSTRING];
	char cws2[SHORTSTRING];
	convertCw(cw1,cws1,SHORTSTRING);
	convertCw(cw2,cws2,SHORTSTRING);
	serverClassifier *sc=findClassifier(serverIP,serverPort);
	if (sc == 0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"no classifier for %08x.%u", serverIP,serverPort);
		return 0;
	}
	if(strcmp(violation,"PRDZERO")==0){
		updateClassifierPRDZERO(sc,cw1,cw2,t,count);
	}
	else if(strcmp(violation,"PRDALLMAX")==0){
		updateClassifierPRDALLMAX(sc,cw1,t,count);
	}
	else if(strcmp(violation,"PRDALLSD2")==0){
		updateClassifierPRDALLSD2(sc,cw1,t,count);
	}
	else if(strcmp(violation,"PRDMAX")==0){
		updateClassifierPRDMAX(sc,cw1,cw2,t,count);
	}
	else if(strcmp(violation,"PRDSD2")==0){
		updateClassifierPRDSD2(sc,cw1,cw2,t,count);
	} else {
		logMessage(stderr,__FUNCTION__,__LINE__,
			"UNKNOWN PRD ALERT %s",violation);
		return 0;
	}
	if ((now-lastClassifyTime)<1){return 0;}
	lastClassifyTime=now;
	char *a=classifyAttack(sc,t);
	if (a != 0){
		++ATTACKS;
		logMessage(stderr,__FUNCTION__,__LINE__,
				"attack: %s",a);
	}
	dumpClassifier(classifierSsv,sc,now);
	return(a);
}
//WHEN get a PRD alert, call one of the above functions to update the rate
//Then evaluate the classifier based on the class rules
