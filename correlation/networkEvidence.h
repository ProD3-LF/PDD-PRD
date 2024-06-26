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
#ifndef NETWORKEVIDENCE_H
#define NETWORKEVIDENCE_H
#define NEINCR 100
#define MAXATTRIBUTIONRULES 20
#include "../common/decay.h"
#include "alert.h"
#include <stdint.h>
#include <sys/types.h>
#define CLEARGUILTTIME 30
#define CLEAREVIDENCETIME 30
extern void addAttributionRule(char *r);
extern void dumpAttributionRules();
typedef struct {
        uint32_t IP;
        size_t count,guiltyCount;
	time_t lastAnomaly,lastGuilty;
        DqDecayData avg[MAX_ALERT_TYPES];
} networkEvidence;
extern void outputGuilty(networkEvidence **ne,size_t *nex,time_t now);
extern networkEvidence *addNe(uint32_t IP,int alertType,size_t count,networkEvidence ***ne,size_t *nex,size_t *nen);
extern char *attributeClients(char *attackName,networkEvidence **ne,size_t nex,time_t now);
extern void attributeOneClient(char *attackName,networkEvidence *ne,time_t now);
#endif
