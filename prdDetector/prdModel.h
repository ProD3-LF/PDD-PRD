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
 * DISTAR Case 38846, cleared November 1, 2023
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */
#ifndef PRDMODEL_H
#define PRDMODEL_H
#include "../common/prdObs.h"
#include "defs.h"
#define Sent 1
#define Received (-1)
#define HIGH 0
#define MED 1
#define LOW 2
extern int maxCw,minCw;
struct {
	long long unsigned int timeStamp[NINT];
	nodeCwCnt obs[NINT];
	size_t obsX;
} typedef currentWindowObs;
struct prdModel_ {
	int low,high;
	int ratio[MAXCW*2][MAXCW*2];
	int allRatio[MAXCW*2];
} typedef prdModel;
struct {
	char name[MEDIUMSTRING];
	long long unsigned int lastCheck;
	int CONZERO,CONSEQMAX,CONSEQ2,CONSEQALLMAX,CONALLSEQ2;
	unsigned int prdCount[MAXCW*2];
	currentWindowObs o;
	uint32_t server;
	uint16_t port;
	prdModel curState;
	size_t prdTotal;
} typedef serverData;
extern long long unsigned int OBSIN;
extern uint64_t comparCurToModel(char *name,unsigned int prdCount[],prdModel *curState, uint64_t now,uint64_t *zCount,uint64_t *errorCount,uint64_t *max,uint64_t *two,uint64_t *allmax,uint64_t *alltwo);
#endif
