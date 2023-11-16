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
#ifndef PRDOBS_H
#define PRDOBS_H
#include <stddef.h>
#include <stdint.h>
#define MAXCW 128
#define PRDOBSTIME 500
struct {
	unsigned long long int lastOutputTime;
	uint32_t serverIP;
	uint16_t serverPort;
	unsigned int prdCountIn[MAXCW];
	unsigned int prdCountOut[MAXCW];
	size_t prdTotal;
} typedef nodeCwCnt;
extern void processTcp(nodeCwCnt *p);
extern void resetCwCnts(nodeCwCnt *nc);
#endif
