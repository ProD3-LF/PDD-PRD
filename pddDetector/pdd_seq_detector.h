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
#ifndef PDD_SEQ_DETECTOR_H
#define PDD_SEQ_DETECTOR_H
#include <stdbool.h>
#pragma once
#define u32 unsigned int
#include "pddModel.h"
#undef assert

typedef enum _Prod3__Severity {
	PROD3__SEVERITY__LOW = 0,
	PROD3__SEVERITY__MEDIUM = 1,
	PROD3__SEVERITY__HIGH = 2,
	PROD3__SEVERITY__ALLCLEAR = 3
} Prod3__Severity;

typedef enum {
	UNINIT,
	MODELFOUND,
	MODELNOTFOUND
} SCMODELSTATE;

enum SCalert {
	SC_ALIEN,
	SC_ANOM,
	SC_START_SHORT,
	SC_START_ALIEN,
	SC_START_ANOM,
	SC_OVERFLOW,
	SC_ALERT_MAX
};

typedef struct {
	time_t firstAlertTime;
	unsigned count;
	char *detail;
} AggInfo;

struct {
	size_t uninterruptedCWCount;
	int detectVec_[MAXSCVEC], detectVecI_;
	int nScs;
	SCMODELSTATE scModelState,startModelState;
	scModel *scm[MAXSCVEC+1];
	scModel *startModel[MAXSCVEC+1];
	int32_t serverIP,clientIP,serverPort,clientPort;
	long long unsigned int lastCwTime;
	bool idle;
} typedef pddDetector;

extern void processCW(pddDetector *d,int cw,
		long long unsigned int obsTime,size_t bucket);
extern void sendPddAlert(uint32_t serverIP,uint32_t serverPort,
	uint32_t clientIP,uint16_t clientPort, enum SCalert type,
	int v[], int n);
extern void initPddDetector(pddDetector *d, uint32_t server,uint16_t serverPort,
	uint32_t client,uint32_t clientPort,long long unsigned int obsTime);
extern ScStatusCode checkVSc(int v[],int n,scModel **sc,int ng);
extern ScStatusCode checkVStart(int v[],int n,scModel **sc,
		int ng);
struct {
	void (*initAlert)();
	void (*flushAlert)();
	void (*closeAlert)();
	void (*sendOverflowAlert)();
	void (*sendAlert)(uint32_t serverIP,uint32_t serverPort,
			uint32_t clientIP, uint16_t clientPort,
			enum SCalert type,int v[],int n);
} typedef callBacks;
extern void (*INITALERT)();
extern void (*FLUSHALERT)();
extern void (*CLOSEALERT)();
extern void (*SENDOVERFLOWALERT)();
extern void (*SENDALERT)(uint32_t serverIP,uint32_t serverPort,uint32_t clientIP,
		uint16_t clientPort, enum SCalert type,int v[],int n);
extern void pddMain(callBacks *c);
/*
	Local Variables:
	mode:c
	c-file-style:"GNU"
	c-backslash-max-column:160
	c-file-offsets:((brace-list-open . 0)(substatement-open . 0))
	comment-column:48
	comment-end:""
	comment-start:"// "
	End:
*/
/* vim: sw=2
 */
#endif
