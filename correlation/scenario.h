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
#ifndef SCENARIO_H
#define SCENARIO_H
#define MAX_GENERIC_SCENARIO 20
#define MAX_SCENARIO_TERMS 10
extern char GENERIC_SCENARIO_FILE[];
#define SCENARIO_MAX 32
extern size_t evaluateGenericScenarios(baseProg *b,time_t now,double score[]);
typedef struct scenario_ {
	char name_[SCENARIO_MAX];
	size_t nTerms_;
	int alertType_[MAX_SCENARIO_TERMS];
	int secs_;
} scenario;
#endif

