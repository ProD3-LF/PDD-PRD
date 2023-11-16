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
#ifndef PDDOBS_H
#define PDDOBS_H
#include <stdint.h>
#include <sys/types.h>
#define MAXOBS 1000
#define MAXOBSTIME 1000 //1 seconds
#define Sent 1
#define Received -1
#define NFIFOS 128
struct {
	uint32_t clientIP,serverIP;
	uint16_t clientPort,serverPort;
	uint8_t cw;
	int8_t direction;
} typedef pddData;
struct {
	size_t pddObsX,bucket;
	long long unsigned timeStamp;
	unsigned int sample;
	pddData pd[MAXOBS];
} typedef pddObs;
#endif
