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
#include "baseProg.h"
#include "correlatorFileDefs.h"
#include "defs.h"
#include "scenario.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
char GENERIC_SCENARIO_FILE[PATH_MAX]=GENERIC_SCENARIO_FILE_DEFAULT;
scenario genericScenario[MAX_GENERIC_SCENARIO];
size_t isInAnyScenario[AR_MAX_ALERT_TYPES];
size_t genericScenarioX=0;
int LONGESTSCENARIO=0;
void dumpGenericScenarios(){
	logMessage(stderr,__FUNCTION__,__LINE__,"\n");
	for(size_t i=0;i<genericScenarioX;++i){
		logMessage(stderr,__FUNCTION__,__LINE__," %s %ld %d:\n",genericScenario[i].name_,
				genericScenario[i].nTerms_,
				genericScenario[i].secs_);
		for(size_t j=0;j<genericScenario[i].nTerms_;++j){
			logMessage(stderr,__FUNCTION__,__LINE__,"  %s ",
				alertType2String(genericScenario[i].alertType_[j]));
		}
		logMessage(stderr,__FUNCTION__,__LINE__,"\n");
	}
}
void initGenericScenario() {
	for(size_t i=0;i<AR_MAX_ALERT_TYPES;++i){isInAnyScenario[i]=0;}
	for(size_t i=0;i<MAX_GENERIC_SCENARIO;++i){
		genericScenario[i].name_[0]='\0';
		genericScenario[i].nTerms_=0;
		genericScenario[i].secs_=0;
		for(int j=0;j<MAX_SCENARIO_TERMS;++j){
			genericScenario[i].alertType_[j]=0;
		}
	}
}
size_t buildScenarioSequence(baseProg *b,scenario *f,int seq[],time_t now){
	size_t isInScenario[AR_MAX_ALERT_TYPES];
	for(size_t i=0;i<AR_MAX_ALERT_TYPES;++i){isInScenario[i]=0;}
	for(size_t i=0;i<f->nTerms_;++i){isInScenario[f->alertType_[i]]=1;}
	size_t k=0;
	for(size_t i=0;i<b->scenarioSequenceX;++i){
		if (isInScenario[b->seq[i].alertType_]!=0){
			if ((now-b->seq[i].alertTime_)<=f->secs_){
				seq[k++]=b->seq[i].alertType_;
			}
		}
	}
	return(k);
}
double evaluateScenario(baseProg *b,scenario *f,time_t now){
	int seq[MAX_SCENARIO_SEQUENCE];
	if (f->nTerms_ == 0){return(0);}
	size_t n=buildScenarioSequence(b,f,seq,now);
	if (n==0){return(0);}
	if (n<f->nTerms_){return(0);}
	size_t i=0;
	for(size_t j=0;j<f->nTerms_;++j){
		for(;i<n;++i){
			if (seq[i] == f->alertType_[j]){
				 break;
			}
		}
		if (i == n){
			 return(0);
		}
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"all FOUND for %08x.%d %s\n",
			b->serverIP,b->serverPort,f->name_);
	return(1);
}
static size_t loadTerm(char *b,scenario *f){
	if (strnlen(b,LONGSTRING) >= LONGSTRING){
		logMessage(stderr,__FUNCTION__,__LINE__,"input scenario too long\n");
		return(0);
	}
	if (sscanf(b,"SEC %d",&f->secs_)==1){ //NOLINT(cert-err34-c)
		if(f->secs_ > LONGESTSCENARIO){LONGESTSCENARIO=f->secs_;}
		return(1);
	}
	if ((f->alertType_[f->nTerms_]=alertType2Int(b))==-1){
		logMessage(stderr,__FUNCTION__,__LINE__,"invalid alert type %s\n",b);
		return(0);
	}
	isInAnyScenario[f->alertType_[f->nTerms_]]++;
	f->nTerms_++;
	return(1);
}
size_t readGenericScenarios(){
	char buf[LONGSTRING];
	int state=0;
	FILE *f=fopen(GENERIC_SCENARIO_FILE,"re");
	if (f==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"ERROR fopen %s\n",
			strerror(errno));
		return(0);
	}
	while(fgets(buf,LONGSTRING,f) != 0){
		if (buf[0] == '#'){continue;}
		if (buf[0] == '\n'){continue;}
		if (state == 0) {
			if (sscanf(buf,
			     "%s {\n",
				genericScenario[genericScenarioX].name_)!=1){
				logMessage(stderr,__FUNCTION__,__LINE__,"ERROR parse in %s\n", buf);
				
			} else{state = 1;}
		} else {
			if (state == 1) {
				if (buf[0] == '}'){
					++genericScenarioX;
					state=0;
				}
				else {
					loadTerm(buf,
			     		    &genericScenario[genericScenarioX]);
				}
			}
		}
	}
	if (state != 0){
		logMessage(stderr,__FUNCTION__,__LINE__,"ERROR incomplete scenario\n");
	}
	fclose(f);
	return(genericScenarioX);
}

size_t evaluateGenericScenarios(baseProg *b,time_t now,double score[]){
	size_t i=0;
	for(;i<genericScenarioX;++i){
		score[i]=evaluateScenario(b,&genericScenario[i],now);
	}
	return(i);
}
void initGenericScenarios(){
	readGenericScenarios();
	dumpGenericScenarios();
}
void pruneScenarioSequence(baseProg *b,time_t now){
	size_t prunePoint=b->scenarioSequenceX+1;
	for(size_t i=0;i<b->scenarioSequenceX;++i){
		if ((now-b->seq[i].alertTime_)<LONGESTSCENARIO){ break;}
		prunePoint=i;
	}
	if (prunePoint==b->scenarioSequenceX+1){return;}
	++prunePoint;
	for(size_t i=prunePoint;i<b->scenarioSequenceX;++i){
		b->seq[i-prunePoint]=b->seq[i];
	}
	b->scenarioSequenceX-=prunePoint;
}
int addAlertToScenarioSequence(baseProg *b,int alertType,time_t alertTime){
	if (isInAnyScenario[alertType]==0){
		 return(0);
	}
	pruneScenarioSequence(b,alertTime);
	if ((b->seq[b->scenarioSequenceX-1].alertType_ == alertType) && 
		(b->seq[b->scenarioSequenceX-1].alertTime_ == alertTime)){
		return(0);
	}
	if (b->scenarioSequenceX == MAX_SCENARIO_SEQUENCE) {
		logMessage(stderr,__FUNCTION__,__LINE__, "no room for alertType %d for %08x.%d\n",
			alertType,b->serverIP,b->serverPort);
		return(0);
	}
	b->seq[b->scenarioSequenceX].alertType_ = alertType;
	b->seq[b->scenarioSequenceX].alertTime_ = alertTime;
	b->scenarioSequenceX++;
	return(1);
}
