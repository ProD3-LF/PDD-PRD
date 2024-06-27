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
#include <stddef.h>
#include <stdint.h>
#ifndef FLAGS_H
#define FLAGS_H
#define FIN 0x1
#define SYN 0x2
#define RST 0x4
#define PSH 0x8
#define ACK 0x10
#define URG 0x20
#define MAXCW 128
#define MAXCW2 256
#define ERROR_STRING 10000
extern int convertCwName(char *cwName);
extern void convertCw(int cw,char *cws,size_t n);
extern int inRequestedRange(int start,int stop,int k);
extern int CWMAPN;
extern int flagToCW[];
extern int CURRENTCONTROLWORD;
struct cwMap_ {
	char controlWordString[MAXCW];
	int cw;
} typedef cwMap;
extern cwMap CWMAP[];
extern void initFlagToCW();
extern unsigned char flagsToInt(char *f);
extern char *flagsAsString(uint8_t f);
extern char *initCwMap();
extern void setMinMaxCw(int *minCw,int *maxCw);
#define COMMONMODELDIR MODELDIR"/common"
#define MODELDIR CENTRAL"/models"
#define CWMAPFILE COMMONMODELDIR"/cwmap"
#endif
