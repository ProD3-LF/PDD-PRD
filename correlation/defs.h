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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#define UNKNOWN_PROTO 0
#define SIMBUFSIZE 1000
#define MAX_SCENARIO_SEQUENCE 1000
#define DEFAULTTHRESHOLD 1.0
#define MAXATTACKS 100
#define SHORTSTRING 32
#define LONGSTRING 512
#define MEDIUMSTRING 128
#define MALLOCPAD 10
int32_t strToPI(char *s);
extern char currentAttackName[][LONGSTRING];
extern size_t currentAttackNameX;
extern size_t ALERTS;
extern size_t ATTACKS;
extern void correlatorMain();
extern int RECORD,DETECT;
extern void configureCorrelator();
extern FILE *ssvFile,*alertsInFile,*alertsOutFile;
extern int LOGIRFD;
extern struct in_addr MYIP;
extern void setMyIp(struct in_addr *ip);
extern int sendToMaster(char *buf,size_t l);
extern void initGenericFormulas();
extern void initGenericScenarios();
extern FILE *alertsRateFile;
extern void initAlertRate();
extern void initBP();
extern int extractFiveTuple(char *a,int n,int nft,
        unsigned int *sIp,unsigned int *dIp,unsigned int *sPort,
                        unsigned int *dPort,unsigned int *protocol,
                        long long *time);
extern int getMode();
extern void setMode(int mode);
extern int parseLine(char *b,size_t n,char *appName,int *alertType,int *score);
extern int checkRate(int type,double threshold);
extern int alertType2Int(char *b);
extern const char* alertType2String(int a);
extern void logIR(char *b, size_t n);
#endif

