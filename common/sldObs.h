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
#ifndef SLDOBS_H
#define SLDOBS_H
#include <stddef.h>
#include <stdint.h>
#define SLDOBSTIME 500
#define NSLDOBS 1000
struct {
	uint32_t serverIP;
	uint16_t serverPort;
	uint32_t clientIP;
	uint16_t clientPort;
	double pTime;
} typedef sldObsOne;
struct {
	size_t x;
	unsigned long long int lastOutputTime;
	sldObsOne o[NSLDOBS];
} typedef sldObs;
#endif
