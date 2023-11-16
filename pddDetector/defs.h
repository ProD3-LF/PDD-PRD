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
#ifndef DEFS_H
#define DEFS_H
#include "../common/pddObs.h"
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#define SHORTSTRING 32
#define LONGSTRING 512
#define MAXCW 128
#define MAXPORTS 65536
#define NSERVD 1
#define TCPIDLETIME 650000
#define ALERTSFILE "/tmp/PDDALERTS"
extern size_t NGRAMSCHECKED;
extern size_t NGRAMSOK;
extern size_t NGRAMSANOM;
extern size_t NGRAMSALIEN;
extern size_t NGRAMSSTARTALIEN;
extern size_t NGRAMSSTARTANOM;
extern size_t NGRAMSSTARTSHORT;
extern size_t NGRAMSABANDONED;
extern size_t CWIN;
extern FILE *ssvFile;
extern FILE *gFile;
extern char *initModels();
extern void freeModels();
extern void freeServerData();
extern unsigned int SAMPLERATE[];
extern unsigned int lastSample[NFIFOS];
extern long long unsigned int sampleChange[];
extern void initSERVD();

extern void processTcp(pddObs *p);
extern void pddInit();
extern int unsignedComp(unsigned int u1,unsigned int u2);
extern void setupThreadObsFd();
extern void flushThreadObsFd();

extern void convertCw(int cw,char *cws,size_t n);
extern int convertCwName(char *cwName);

extern FILE *alertsFile;
extern char hostName[];
extern void flushAlerts();
extern void xmitQueuedAlerts();
extern void initAlert();
extern void closeAlert();
extern bool adDetect,adRecord;
extern void addKnownServer(uint16_t p,in_addr_t ip);

#endif
