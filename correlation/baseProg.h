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
#ifndef BASEPROG_H
#define BASEPROG_H
#include "../common/decay.h"
#include "alert.h"
#include "defs.h"
#include "networkEvidence.h"
#define AR_MAX_ALERT_TYPES MAX_ALERT_TYPES
typedef struct scenarioSequence_ {
	int alertType_;
	time_t alertTime_;
} scenarioSequence;
typedef struct baseProg_ {
	uint32_t serverIP;
	unsigned int serverPort;
	size_t level;
	size_t scenarioSequenceX;
	scenarioSequence seq[MAX_SCENARIO_SEQUENCE];
        DqDecayData avg[AR_MAX_ALERT_TYPES];
        double max[AR_MAX_ALERT_TYPES];
        double threshold[AR_MAX_ALERT_TYPES];
        time_t lastAction,lastIssuedAlert;
	int violationsListN;
	char *violationsList;
	double nrm,hrm,pm,um;
	networkEvidence **ne;
	size_t nex,nen;
} baseProg;
extern void pruneOldEvidence(baseProg *b);
#define MAXBP 1000
#define BPMAXAGE 60
#endif
