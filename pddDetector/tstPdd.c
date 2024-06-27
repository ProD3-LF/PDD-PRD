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
#include "defs.h"
#include "pdd_seq_detector.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
extern void initAlertCorr();
extern 	void flushAlertsCorr();
extern 	void closeAlertCorr();
extern 	void sendOverFlowAlertCorr();
extern 	void sendPddAlertCorr(uint32_t serverIP,uint32_t serverPort,
			uint32_t clientIP, uint16_t clientPort,
			enum SCalert type,int v[],int n);
extern void initAlert();
extern 	void flushAlert();
extern 	void closeAlert();
extern 	void sendOverFlowAlert();
extern 	void sendPddAlert(uint32_t serverIP,uint32_t serverPort,
			uint32_t clientIP, uint16_t clientPort,
			enum SCalert type,int v[],int n);
int main(){
	callBacks c;
	c.initAlert = &initAlertCorr;
	c.flushAlert = &flushAlertsCorr;
	c.sendAlert = &sendPddAlertCorr;
	c.sendAlert = &sendPddAlertCorr;
	c.sendOverflowAlert = &sendOverFlowAlert;
	pddMain(&c);
}
