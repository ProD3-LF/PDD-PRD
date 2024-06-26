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
#ifndef ALERT_H
#define ALERT_H
#include <stdint.h>
#include <sys/types.h>
#define SHORTSTRING 32
#define LONGSTRING 512
typedef struct alertCommon_ {
        int type,impact,confidence;
        uint64_t start;
	size_t count;
	uint32_t clientIP,serverIP;
	unsigned int clientPort,serverPort;
	char violation[SHORTSTRING];
	time_t alertTime;

} alertCommon;
typedef struct PRDALERT_ {
	char violation[SHORTSTRING];
	uint32_t serverIP;
	unsigned int serverPort;
	time_t alertTime;
        uint64_t start;
	size_t count;
	char details[];
} PRDALERT;
typedef struct PDDALERT_ {
	char violation[SHORTSTRING];
	int impact;
	uint32_t clientIP,serverIP;
	unsigned int clientPort,serverPort;
	time_t alertTime;
        uint64_t start;
	size_t count;
	char details[LONGSTRING];
} PDDALERT;
typedef struct SLDALERT_ {
	char violation[SHORTSTRING];
	int impact;
	uint32_t serverIP;
	uint16_t serverPort;
	time_t alertTime;
	size_t count;
	char details[];
} SLDALERT;
typedef struct ALERT_ {
	char message_name[SHORTSTRING];
	union {
		PRDALERT *prdAlert;
		PDDALERT *pddAlert;
		SLDALERT *sldAlert;
	} t;
} ALERT;
#define PDDALIEN 0
#define PDDALIEN 0
#define PDDANOM 1
#define PDDSTARTSHORT 2
#define PDDSTARTALIEN 3
#define PDDSTARTANOM 4
#define PDDOVERFLOW 5
#define PRDZERO 6
#define PRDALLMAX 7
#define PRDALLSD2 8
#define PRDMAX 9
#define PRDALIEN 10
#define PRDSD2 11
#define SLDTIMEOK 12
#define SLDTIMEMAXLOW 13
#define SLDTIMELOW 14
#define SLDTIMEHIGH 15
#define SLDTIMEMAXHIGH 16
#define SLDTIMEZERO 17
#define SLDCOUNTOK 18
#define SLDCOUNTMAXLOW 19
#define SLDCOUNTLOW 20
#define SLDCOUNTHIGH 21
#define SLDCOUNTMAXHIGH 22
#define SLDCOUNTZERO 23
#define SLDALERTANOMMAX 24
#define MAX_ALERT_TYPES 25
//Abstract alert classificications
#define NRM 0 //networkhost resource misuse
#define HRM 1 //nost resource misuse
#define PM 2 //protocol misuse
#define UM 3 //unknown misuse
#endif
