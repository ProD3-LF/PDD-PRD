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
 * DISTAR Case 38846, cleared November 1, 2023
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */
#include "../common/logMessage.h"
#include "../common/prdObs.h"
#include "defs.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
FILE *alertsFile=0;
void initAlert(){
	if ((alertsFile=fopen(ALERTSFILE,"we"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"%s.%d cannot open %s",
			ALERTSFILE);
		exit(-1);
	}
}
void flushAlerts(){
	fflush(alertsFile);
}
void closeAlert(){
	fclose(alertsFile);
}

void sendPrdAlert(uint32_t serverIP,uint16_t serverPort,char *violation){
	char serverIPs[SHORTSTRING];
	struct in_addr t;
	serverPort=ntohs(serverPort);
	t.s_addr=serverIP;
        strncpy(serverIPs,inet_ntoa(t),sizeof(serverIPs));
	fprintf(alertsFile,"%lu %15s.%-6u %s\n",
			time(0),serverIPs,ntohs(serverPort),violation);
	(*FLUSHALERT)();
}
