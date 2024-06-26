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
#ifndef FORMULA_H
#define FORMULA_H
#define MAX_GENERIC_FORMULA 20
#define MAX_FORMULA_TERMS 10
extern char GENERIC_FORMULA_FILE[];
#define DEFAULT_ALERTSRATE 2
#define ALERTSRATE_FACTOR 1.5
#define AGEFACTOR 30
#define FORMULA_MAX 32
#define EITHRESHOLD 0.1
extern size_t evaluateGenericFormulas(baseProg *b,time_t now,double score[]);
typedef struct formula_ {
	char name_[FORMULA_MAX];
	int nTerms_;
	int alertType_[MAX_FORMULA_TERMS];
	double threshold_[MAX_FORMULA_TERMS];
	double coefficient_[MAX_FORMULA_TERMS];
} formula;
#endif

