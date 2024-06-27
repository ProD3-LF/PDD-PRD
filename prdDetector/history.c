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
#include "history.h"
#include  <stdio.h>
#include  <string.h>
#include  <time.h>
alertHist ALLMAXHist[MAXHIST];
alertHist MAXHist[MAXHIST];
alertHist ALL2STDHist[MAXHIST];
alertHist TWOSTDHist[MAXHIST];
alertHist ZEROHist[MAXHIST];
size_t ALLMAXHistN=0;
size_t MAXHistN=0;
size_t ALL2STDHistN=0;
size_t TWOSTDHistN=0;
size_t ZEROHistN=0;
void alertHistToString(alertHist *h,char *b,int n){
	snprintf(b,n,"(%lu,%d,%d,%lu)",h->aTime,h->cw1,h->cw2,h->count);
}
void outputHist(alertHist h[],size_t *nh,char *d,size_t dn){
	bzero(d,dn);
	size_t tdn=dn;
	for(size_t i=0;i<*nh;++i){
		char b[64];
		alertHistToString(&h[i],b,64);
		strncat(d,b,dn);
		dn-=strlen(b);
		if (dn > tdn){
			fprintf(stderr,"%s.%d overflow\n",
					__FUNCTION__,__LINE__);
			break;
		}
	}
	*nh=0;
}
void addAllMaxHist(time_t now,int cw1){
	for (size_t i=0;i<ALLMAXHistN;++i){
		if ((ALLMAXHist[i].aTime==now) && (ALLMAXHist[i].cw1==cw1)){
			++ALLMAXHist[i].count;
			return;
		}
	}
	if (ALLMAXHistN == MAXHIST){
		fprintf(stderr,"%s.%d too much history\n",
				__FUNCTION__,__LINE__);
		return;
	}
	ALLMAXHist[ALLMAXHistN].aTime=now;
	ALLMAXHist[ALLMAXHistN].cw1=cw1;
	ALLMAXHist[ALLMAXHistN].cw2=0;
	ALLMAXHist[ALLMAXHistN].count=1;
	++ALLMAXHistN;
}
void addAll2STDHist(time_t now,int cw1){
	for (size_t i=0;i<ALL2STDHistN;++i){
		if ((ALL2STDHist[i].aTime==now) && (ALL2STDHist[i].cw1==cw1)){
			++ALL2STDHist[i].count;
			return;
		}
	}
	if (ALL2STDHistN == MAXHIST){
		fprintf(stderr,"%s.%d too much history\n",
				__FUNCTION__,__LINE__);
		return;
	}
	ALL2STDHist[ALL2STDHistN].aTime=now;
	ALL2STDHist[ALL2STDHistN].cw1=cw1;
	ALL2STDHist[ALL2STDHistN].cw2=0;
	ALL2STDHist[ALL2STDHistN].count=1;
	++ALL2STDHistN;
}
void addMaxHist(time_t now,int cw1,int cw2){
	for (size_t i=0;i<MAXHistN;++i){
		if ((MAXHist[i].aTime==now) && (MAXHist[i].cw1==cw1)){
			++MAXHist[i].count;
			return;
		}
	}
	if (MAXHistN == MAXHIST){
		fprintf(stderr,"%s.%d too much history\n",
				__FUNCTION__,__LINE__);
		return;
	}
	MAXHist[MAXHistN].aTime=now;
	MAXHist[MAXHistN].cw1=cw1;
	MAXHist[MAXHistN].cw2=cw2;
	MAXHist[MAXHistN].count=1;
	++MAXHistN;
}
void add2StdHist(time_t now,int cw1,int cw2){
	for (size_t i=0;i<TWOSTDHistN;++i){
		if ((TWOSTDHist[i].aTime==now) && (TWOSTDHist[i].cw1==cw1) && (TWOSTDHist[i].cw2==cw2)){
			++TWOSTDHist[i].count;
			return;
		}
	}
	if (TWOSTDHistN == MAXHIST){
		fprintf(stderr,"%s.%d too much history\n",
				__FUNCTION__,__LINE__);
		return;
	}
	TWOSTDHist[TWOSTDHistN].aTime=now;
	TWOSTDHist[TWOSTDHistN].cw1=cw1;
	TWOSTDHist[TWOSTDHistN].cw2=cw2;
	TWOSTDHist[TWOSTDHistN].count=1;
	++TWOSTDHistN;
}
void addZeroHist(time_t now,int cw1,int cw2){
	for (size_t i=0;i<ZEROHistN;++i){
		if ((ZEROHist[i].aTime==now) && (ZEROHist[i].cw1==cw1) && (ZEROHist[i].cw2==cw2)){
			++ZEROHist[i].count;
			return;
		}
	}
	if (ZEROHistN == MAXHIST){
		fprintf(stderr,"%s.%d too much history\n",
				__FUNCTION__,__LINE__);
		return;
	}
	ZEROHist[ZEROHistN].aTime=now;
	ZEROHist[ZEROHistN].cw1=cw1;
	ZEROHist[ZEROHistN].cw2=cw2;
	ZEROHist[ZEROHistN].count=1;
	++ZEROHistN;
}
