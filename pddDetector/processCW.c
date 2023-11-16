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
#include "../common/pddObs.h"
#include "defs.h"
#include "pddFileDefs.h"
#include "pddModel.h"
#include "pdd_seq_detector.h"
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <time.h>
size_t CWIN=0;
size_t NGRAMSCHECKED=0;
size_t NGRAMSOK=0;
size_t NGRAMSANOM=0;
size_t NGRAMSALIEN=0;
size_t NGRAMSSTARTANOM=0;
size_t NGRAMSSTARTALIEN=0;
size_t NGRAMSSTARTSHORT=0;
size_t NGRAMSABANDONED=0;
static FILE *threadObsFd[NFIFOS];
void flushThreadObsFd(){
	for(size_t i=0;i<NFIFOS;++i){
		logMessage(stderr,__FUNCTION__,__LINE__,"flushed %lu",i);
		fflush(threadObsFd[i]);
	}
}
void setupThreadObsFd(){
	char fileName[PATH_MAX];
	for(size_t i=0;i<NFIFOS;++i){
		snprintf(fileName,sizeof(fileName),
				"%s.thread%lu",MASTERTRAINFILE,i);
		if ((threadObsFd[i]=fopen(fileName,"we"))==0){
			logMessage(stderr,__FUNCTION__,__LINE__,
				"fopen(%s): %s",fileName,strerror(errno));
			exit(-1);
		}
	}
}
void learnSequence(pddDetector *d,int n,size_t bucket, char *type){
	fprintf(threadObsFd[bucket],"TRAIN TCP.TRAIN.%s.%d ",type,n);
	for (size_t i=0;i<n;++i){
		fprintf(threadObsFd[bucket],"%d ",
			d->detectVec_[d->detectVecI_-n+i]);
	}
	fprintf(threadObsFd[bucket],"\n");
}
void shiftVec(pddDetector *d){
	for(int i=0;i<MAXSCVEC-1;++i){d->detectVec_[i] = d->detectVec_[i+1];}
	d->detectVecI_=MAXSCVEC-1;
}
extern void reusePddDetector(pddDetector *d,int cw,long long unsigned int now);
void checkForReuse(long long unsigned int now,pddDetector *d,int cw){
	//if already marked as abandoned, just reuse it
	if (d->idle==true) {
		reusePddDetector(d,cw,now);
		return;
	}
	if ((now-d->lastCwTime)<TCPIDLETIME){return;}
	if (d->startModel[1]==0){return;}
	//The session has been idle a long time.
	//If the prior session has not completed the start sequence
	//issue an alert since many attacks cause the start sequence to fail.
	if (d->nScs < MAXSCVEC){
		NGRAMSSTARTSHORT++;
		(*SENDALERT)(d->serverIP,d->serverPort,d->clientIP,
			d->clientPort, SC_START_SHORT,
			&d->detectVec_[d->detectVecI_-d->nScs],
			d->nScs);
	}
	//If the new CW is one that starts a new session
	//assume the client is reusing the client port for a new session
	if (checkVStart(d->detectVec_, d->detectVecI_, 
			&d->startModel[1],1)==SC_POL_OK){
		reusePddDetector(d,cw,now);
		return;
	}
	//But if the new CW is not one that starts a new session assume
	//the session was just idle a long time and continue despite
	//the long gap
	NGRAMSABANDONED++;
	logMessage(stderr,__FUNCTION__,__LINE__,
			"ABANDONED: %llu %d %llu %d %08x:%d %08x:%d",
		now,cw,now-d->lastCwTime,d->nScs,ntohl(d->serverIP),
		ntohs(d->serverPort),ntohl(d->clientIP),ntohs(d->clientPort));
}
static size_t alertsPer=0,lastCum=0,currentCum=0;
void processCW(pddDetector *d, int cw,
		long long unsigned int obsTime,
		size_t bucket) {
	static time_t lastSsvUpdate=0;
	int lowestAnomAlert=MAXSCVEC+1;
	d->nScs++;
	d->uninterruptedCWCount++;
	d->detectVec_[d->detectVecI_++]=cw;
	d->lastCwTime=obsTime;
	checkForReuse(obsTime,d,cw);
	++CWIN;
	if ((d->nScs<=MAXSCVEC) && (d->nScs==d->uninterruptedCWCount)){
		for(int i=d->nScs;i<=MAXSCVEC;++i){
			if (i>d->nScs){break;}
			if (d->startModel[i]==0){continue;}
			++NGRAMSCHECKED;
			switch(checkVStart(d->detectVec_,d->detectVecI_,
				&d->startModel[i],i)){
			case SC_POL_OK:
				++NGRAMSOK;
				break;
			case SC_POL_ANOMALOUS:
				++NGRAMSSTARTANOM;
				if (adRecord == true){
					learnSequence(d,i,bucket,"START");}
				if (i<lowestAnomAlert){
					(*SENDALERT)(d->serverIP,d->serverPort,
						d->clientIP,d->clientPort,
						SC_START_ANOM,
						&d->detectVec_[d->detectVecI_-i],i);
					lowestAnomAlert=i;
				}
				break;
			case SC_POL_ALIEN:
				++NGRAMSSTARTALIEN;
				if (adRecord == true){
					learnSequence(d,i,bucket,"START");}
				(*SENDALERT)(d->serverIP,d->serverPort,
					d->clientIP,d->clientPort,
				       	SC_START_ALIEN, 
					&d->detectVec_[d->detectVecI_-1],1);
				break;
			case SC_POL_ERROR:
				logMessage(stderr,__FUNCTION__,__LINE__,
						"BAD RETURN");
						
			}
		}
	}
	if ((d->nScs<=MAXSCVEC) && (d->nScs!=d->uninterruptedCWCount)){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"start sequence interrupted");
		return;
	}
	for(int i=0;i<=MAXSCVEC;++i){
		if (i>d->nScs){break;}
		if (d->scm[i] == 0){continue;}
		if (d->detectVecI_ < i){
			++NGRAMSOK;
			continue;
		}
		if (i>d->uninterruptedCWCount){
			logMessage(stderr,__FUNCTION__,__LINE__,
				"sequence interrupted %d %d %d",
				i,d->uninterruptedCWCount,d->nScs);
		       	break;
		}
		++NGRAMSCHECKED;
		switch(checkVSc(d->detectVec_,d->detectVecI_,&d->scm[i],i)){
		case SC_POL_OK:
			++NGRAMSOK;
			break;
		case SC_POL_ANOMALOUS:
			++NGRAMSANOM;
			if (adRecord == true){learnSequence(d,i,bucket,"INCR");}
			if (i<lowestAnomAlert){
				(*SENDALERT)(d->serverIP,d->serverPort,
					d->clientIP,d->clientPort,
					SC_ANOM,
					&d->detectVec_[d->detectVecI_-i],i);
				lowestAnomAlert=i;
			}
			break;
		case SC_POL_ALIEN:
			++NGRAMSALIEN;
			if (adRecord == true){learnSequence(d,i,bucket,"INCR");}
			(*SENDALERT)(d->serverIP,d->serverPort,d->clientIP,
				d->clientPort,
			       	SC_ALIEN, 
				&d->detectVec_[d->detectVecI_-1],1);
			break;
		case SC_POL_ERROR:
			logMessage(stderr,__FUNCTION__,__LINE__,"BAD RETURN");
		}
	}
	if (d->detectVecI_==MAXSCVEC){shiftVec(d);}
	if ((time(0)-lastSsvUpdate) >= 1){
		currentCum=(NGRAMSANOM+NGRAMSALIEN+NGRAMSSTARTANOM+
				NGRAMSSTARTALIEN+NGRAMSSTARTSHORT+
				NGRAMSABANDONED);
		alertsPer=currentCum-lastCum;
		lastCum=currentCum;
		lastSsvUpdate=time(0);
		fprintf(ssvFile, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
				time(0),CWIN,NGRAMSCHECKED,NGRAMSOK,
				NGRAMSANOM,NGRAMSALIEN,NGRAMSSTARTANOM,
				NGRAMSSTARTALIEN,NGRAMSSTARTSHORT,
				NGRAMSABANDONED);
		fprintf(stdout, "%10lu %10lu %10lu\r",
				time(0),CWIN,
				NGRAMSANOM+NGRAMSALIEN+NGRAMSSTARTANOM+
				NGRAMSSTARTALIEN+NGRAMSSTARTSHORT+
				NGRAMSABANDONED);
		fprintf(gFile,"%lu\n",alertsPer);
		fflush(gFile);
		fflush(stdout);
		fflush(ssvFile);
	}
}
