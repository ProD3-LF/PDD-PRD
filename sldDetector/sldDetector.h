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
#ifndef SLDDETECTOR_H
#define SLDDETECTOR_H
#include "../common/decay.h"
#include "defs.h"
#include <stdint.h>
struct {
	uint32_t clientIP;
	uint16_t clientPort;
	DqDecayData pAvg;
	int state;
	time_t lastPacket;
	double lastPacketTime;
	double startActive;
	size_t activeCount;
} typedef sessionData;
struct {
	uint32_t server;
	uint16_t port;
	long long unsigned int lastCheck;
	sessionData **SD[MAXPORTS+1];
	size_t NSD[MAXPORTS+1],DSX[MAXPORTS+1];
} typedef serverData;
#endif
