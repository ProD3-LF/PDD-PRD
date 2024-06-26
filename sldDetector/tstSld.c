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
#include "defs.h"
#include "sldAlertTypes.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
extern void initAlertCorr();
extern void flushAlertsCorr();
extern void closeAlertCorr();
extern void sendOverFlowAlertCorr();
extern void sendSldAlertCorr(uint32_t serverIP,uint32_t serverPort,SLDALERTANOM type, char *details);
extern void initAlert();
extern void flushAlerts();
extern void closeAlert();
extern void sendOverFlowAlert();
extern void sendSldAlert(uint32_t serverIP,uint32_t serverPort,SLDALERTANOM type,
		char *details);
int main(){
	callBacks c;
	c.initAlert = &initAlertCorr;
	c.flushAlert = &flushAlertsCorr;
	c.sendAlert = &sendSldAlertCorr;
	c.closeAlert = &closeAlertCorr;
	sldMain(&c);
}
