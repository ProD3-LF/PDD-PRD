#/* SPDX-License-Identifier: Apache-2.0 */
#/* Copyright (c) 2012-2023 Applied Communication Sciences
# * (now Peraton Labs Inc.)
# *
# * This software was developed in work supported by the following U.S.
# * Government contracts:
# *
# * HR0011-20-C-0160 HR0011-16-C-0061
# * 
# *
# * Any opinions, findings and conclusions or recommendations expressed in
# * this material are those of the author(s) and do not necessarily reflect
# * the views, either expressed or implied, of the U.S. Government.
# *
# * DoD Distribution Statement A
# * Approved for Public Release, Distribution Unlimited
# *
# * DISTAR Case 38846, cleared November 1, 2023
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# * http://www.apache.org/licenses/LICENSE-2.0
# */
#include <time.h>
struct {
	time_t aTime;
	int cw1,cw2;
	size_t count;
} typedef alertHist;
#define MAXHIST 1000
extern alertHist ALLMAXHist[];
extern alertHist MAXHist[];
extern alertHist ALL2STDHist[];
extern alertHist TWOSTDHist[];
extern alertHist ZEROHist[];
extern size_t ALLMAXHistN;
extern size_t MAXHistN;
extern size_t ALL2STDHistN;
extern size_t TWOSTDHistN;
extern size_t ZEROHistN;
extern void outputHist(alertHist h[],size_t *hn,char *d,size_t dn);
extern void alertHistToString(alertHist *h,char *b,int n);
extern void addAllMaxHist(time_t now,int cw1);
extern void addAll2STDHist(time_t now,int cw1);
extern void addMaxHist(time_t now,int cw1,int cw2);
extern void add2StdHist(time_t now,int cw1,int cw2);
extern void addZeroHist(time_t now,int cw1,int cw2);
