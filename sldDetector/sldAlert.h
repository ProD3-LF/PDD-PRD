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
#ifndef SLDALERT_H
#define SLDALERT_H
#include "sldAlertTypes.h"
#include <stdint.h>
extern void alertDetailsToString(uint32_t serverIP,uint16_t serverPort,char **maxTimeLow,char **timeLow,
	char **timeHigh,char **maxTimeHigh, char **timeZero,
	char **maxCountLow,char **countLow,char **countHigh,
	char **maxCountHigh, char **countZero);
extern char *sldTimeAlertToName(SLDANOM a);
extern char *sldCountAlertToName(SLDANOM a);
extern char *sldAlertAnomToName(SLDALERTANOM a);
extern void resetSldAlerts();
extern void dumpSldAlerts();
extern void outputSsv(double obsTime);
extern void addSldTimeAlert(uint32_t server,uint16_t port,uint32_t clientIP,SLDANOM a);
extern void addSldCountAlert(uint32_t server,uint16_t port,uint32_t clientIP,SLDANOM a);
#endif
