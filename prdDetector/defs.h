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
#ifndef DEFS_H
#define DEFS_H
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
extern time_t startTime;
extern FILE *ssvFile;
extern FILE *gFile;
extern char hostName[];
extern bool adDetect,adRecord;
struct {
	void (*initAlert)();
	void (*flushAlert)();
	void (*closeAlert)();
	void (*sendAlert)(uint32_t serverIP,uint16_t serverPort,
			char *violation);
} typedef callBacks;
extern void prdMain(callBacks *c);
extern void initSD(uint32_t serverIP,uint16_t serverPort);
extern void initSERVD();
extern void initKnownServerPorts();
extern int initModels();
extern void initCounts();
extern void (*INITALERT)();
extern void (*FLUSHALERT)();
extern void (*CLOSEALERT)();
extern void (*SENDALERT)(uint32_t serverIP,uint16_t serverPort,char *violation);
#define LONGSTRING 512
#define MEDIUMSTRING 64
#define SHORTSTRING 32
#define ALERTSFILE "/tmp/PRDALERTS"
#define MAXCLASSRULES 10
#define MAXPORTS 65536
#define CORETHRESH 50
#define ZPROBTHRESH 50
#define MAXFILES 6000000
#define NSERVD 2
#define PRDOBSTIME 500
#define PRDOBSTIMESLOP (PRDOBSTIME/2)
#define PRDCHECKTIME 30000
#define PRDALERTINTERVAL 1000
#define NINT (PRDCHECKTIME/PRDOBSTIME)
#define prdDetectorBufferSize 1
extern size_t CWIN;
extern void updateStats(int allmax,int alltwo,int max,int two,int zero,char *name);
extern long long unsigned int msecTime();
extern void addKnownServer(uint16_t p,in_addr_t ip);
extern bool knownServer(uint16_t p,in_addr_t ip);
extern void configurePDR();
extern void prdInit();
extern void *configClient;
extern char * convert();
#endif
