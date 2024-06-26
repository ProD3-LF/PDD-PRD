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
#ifndef DEFS_H
#define DEFS_H
#include "sldAlertTypes.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#define MAXPORTS 65535
#define NSERVD 1
extern double TIME0,TIME100,TIME03,TIME97;
extern double COUNT0,COUNT100,COUNT03,COUNT97;
extern void initSERVD();
extern void sldInit();
extern void addKnownServer(uint16_t p,in_addr_t ip);
extern time_t startTime;
extern FILE *ssvFile;
extern FILE *gFile;
extern FILE *obsFile;
extern char hostName[];
extern long long unsigned ANOMALIES;
#define ALERTSFILE "/tmp/SLDALERTS"
extern bool adDetect,adRecord;
struct {
	void (*initAlert)();
	void (*flushAlert)();
	void (*closeAlert)();
	void (*sendAlert)(uint32_t serverIP,uint32_t serverPort, SLDALERTANOM type,
		char *details);
} typedef callBacks;
extern void sldMain(callBacks *c);
extern void initSD(uint32_t serverIP,uint16_t serverPort);
extern int initModel();
extern uint16_t strToU16(char *s);
extern void (*INITALERT)();
extern void (*FLUSHALERT)();
extern void (*CLOSEALERT)();
extern void (*SENDALERT)(uint32_t serverIP,uint32_t serverPort,SLDALERTANOM type,
		char *details);
#define LONGSTRING 512
#define MEDIUMSTRING 64
#define SHORTSTRING 32
#define ALERTSFILE "/tmp/SLDALERTS"
#endif
