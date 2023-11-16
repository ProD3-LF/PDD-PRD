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
#define _GNU_SOURCE
#include "../common/logMessage.h"
#include "defs.h"
#include "pdd_seq_detector.h"
#include "pddModel.h"
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>
#define MAX_BP 50000
void reusePddDetector(pddDetector *d,int scNum,long long unsigned int now){
	d->nScs=1;
	d->uninterruptedCWCount=1;
	for(size_t i=0;i<MAXSCVEC;++i){ d->detectVec_[i]=0;}
	d->detectVecI_=MAXSCVEC-1;
	d->detectVec_[d->detectVecI_++] = scNum;
	d->lastCwTime=now;
	d->idle=false;
	logMessage(stderr,__FUNCTION__,__LINE__,"reusing PDD detector %08x:%d %08x:%d %d",
		d->serverIP,d->serverPort,d->clientIP,d->clientPort,scNum);
}
void initPddDetector(pddDetector *d, uint32_t server,uint16_t serverPort,
	uint32_t client,uint32_t clientPort,long long unsigned int obsTime){
	int i=0;
	memset(d,0,sizeof(*d));
	d->serverIP=server;
	d->serverPort=serverPort;
	d->clientIP=client;
	d->clientPort=clientPort;
	d->scModelState=d->startModelState=UNINIT;
	d->lastCwTime=obsTime;
	d->idle=false;
	for(i=0;i<MAXSCVEC-1;++i){d->scm[i]=TCPMODEL[i];}
	for(i=0;i<MAXSCVEC-1;++i){d->startModel[i]=TCPSTARTMODEL[i];}
	d->detectVecI_=i;
}
