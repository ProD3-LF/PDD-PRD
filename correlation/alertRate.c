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
#include "alert.h"
#include "alertRate.h"
#include <ctype.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
alertRate AR[MAX_ALERT_TYPES];
void initAlertRate(){
	for(int i=0;i<MAX_ALERT_TYPES;++i){
		for(int j=0;j<MAX_ALERTS;++j){
			AR[i].aTime[j]=0.0;
		}
		AR[i].nAlerts=0;
	}
}
#define USECMULT (double).000001;
double time2double(struct timeval *t){
	double d=(double)t->tv_sec+(double)t->tv_usec*USECMULT;
	return(d);
}
void updateAlertRate(alertRate *ar,double now){
	if (ar->nAlerts < MAX_ALERTS) {
		ar->aTime[ar->nAlerts++]=now;
	} else {
		int i=0;
		for(;i<ar->nAlerts-1;++i){
			ar->aTime[i] = ar->aTime[i+1];
		}
		ar->aTime[i] = now;
	}
	int p = 0;
	for(int i=0;i<ar->nAlerts;++i){
		if ((now - ar->aTime[i]) > ALERT_TIME_WINDOW){
			++p;
		}
		else{break;}
	}
	if (p == 0){return;}
	for(int i=0;i<ar->nAlerts-p;++i){
		ar->aTime[i] = ar->aTime[i+p];
	}
	ar->nAlerts -= p;
}
	
int checkRate(int type,double threshold){
	struct timeval now;
	double nowd=0;
	gettimeofday(&now,0);
	updateAlertRate(&AR[type],(nowd=time2double(&now)));
	if (AR[type].nAlerts < (int)threshold){return(0);}
	double elapsed = nowd - AR[type].aTime[0];
	if (elapsed == 0){return(0);}
	if (((double)AR[type].nAlerts/elapsed) > threshold){return(1);}
	return(0);
}
