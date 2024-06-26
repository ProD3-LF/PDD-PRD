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
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#define MAXSERVER 10
#define MAXCLASSRULES 20
extern void initClassifier();
extern char *updateClassifierSld(uint32_t serverIP,unsigned int serverPort,int type,size_t count,time_t t);
extern char *updateClassifierPdd(uint32_t serverIP,unsigned int serverPort,char *violation,size_t count,time_t t);
extern char *updateClassifierPrd(uint32_t serverIP,unsigned int serverPort,char *violation,time_t t,int cw1,int cw2,int count);
extern void addClassRule(char *r);
extern void dumpClassRules();
struct {
	in_addr_t sIpAddr;
	uint16_t sPort;
	DqDecayData avgSLD_TIME_OK;
        DqDecayData avgSLD_TIME_MAXLOW;
        DqDecayData avgSLD_TIME_LOW;
        DqDecayData avgSLD_TIME_HIGH;
        DqDecayData avgSLD_TIME_MAXHIGH;
        DqDecayData avgSLD_TIME_ZERO;
        DqDecayData avgSLD_COUNT_OK;
        DqDecayData avgSLD_COUNT_MAXLOW;
        DqDecayData avgSLD_COUNT_LOW;
        DqDecayData avgSLD_COUNT_HIGH;
        DqDecayData avgSLD_COUNT_MAXHIGH;
        DqDecayData avgSLD_COUNT_ZERO;
        DqDecayData avgSLD_ALERTANOMMAX;
	DqDecayData avgPDDALIEN;
	DqDecayData avgPDDANOM;
	DqDecayData avgPDDSTARTSHORT;
	DqDecayData avgPDDSTARTALIEN;
	DqDecayData avgPDDSTARTANOM;
	DqDecayData avgPRDZERO[MAXCW*2][MAXCW*2];
	DqDecayData avgPRDALLMAX[MAXCW*2];
	DqDecayData avgPRDALLSD2[MAXCW*2];
	DqDecayData avgPRDSD2[MAXCW*2][MAXCW*2];
	DqDecayData avgPRDMAX[MAXCW*2][MAXCW*2];
} typedef serverClassifier;
extern serverClassifier SC[];
