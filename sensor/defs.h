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
#include <stdbool.h>
#define MAXSERVER 10
extern void flushBuckets();
extern void closeFifos();
extern void handlePDDObs(in_addr_t serverIP,in_addr_t clientIP,uint32_t serverPort, uint32_t clientPort,uint8_t flag,int8_t direction);
extern void handlePRDObs(double obsTime,in_addr_t serverIP,uint32_t serverPort,uint8_t flag,int8_t direction);
extern void initBucket();
extern bool knownPrdServer(uint16_t p,in_addr_t ip);
extern bool knownPddServer(uint16_t p,in_addr_t ip);
extern long long unsigned int msecTime();
extern void initFifos();
extern void pddConfig();
extern void prdConfig();
extern void setupNC();
extern int parseTcpLine(char *b,double *obsTime,in_addr_t *serverPDDIP,
                in_addr_t *clientPDDIP,unsigned int *serverPDDPort,
                unsigned int *clientPDDPort, in_addr_t *serverPRDIP,
                unsigned int *serverPRDPort,
                uint8_t *flag,int8_t *direction);
extern int parseTcpLinePrd(char *b,double *obsTime,in_addr_t *serverIP,
	unsigned int *serverPort,uint8_t *flag, int8_t *direction);
extern int parseTcpLinePdd(char *b,double *obsTime,in_addr_t *serverIP,in_addr_t *clientIP,
	unsigned int *serverPort,unsigned int *clientPort,uint8_t *flag,
	int8_t *direction);
