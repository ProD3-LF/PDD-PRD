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
#include "../common/flags.h"
#include "../common/flags.h"
#include "../common/logMessage.h"
#include "defs.h"
#include "sldFileDefs.h"
#include "sldLog.h"
#include "sldModel.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#define LM(...) logMessage(stderr,__FUNCTION__,__LINE__,__VA_ARGS__)
/*void logAnomaly(long long unsigned int now,char * type, char *name,int cw1,
		int cw2,int modelMax,int modelMean,int modelStdDev,
		int curRatio,int dev){
	char cw1s[SHORTSTRING],cw2s[SHORTSTRING];
	convertCw(cw1,cw1s,SHORTSTRING);
	convertCw(cw2,cw2s,SHORTSTRING);
#ifdef PUBLIC
	logMessage(stderr,__FUNCTION__,__LINE__,
		"%lld %s %s %s/%s model(maxdev %d mean %d stddev %d) current(ratio %d dev %d)\n",
		now,name,type,cw1s,cw2s,modelMax,modelMean,modelStdDev,curRatio,dev);
#else
	pd3log_error(
		"%lld %s %s %s/%s model(maxdev %d mean %d stddev %d) current(ratio %d dev %d)\n",
		now,name,type,cw1s,cw2s,modelMax,modelMean,modelStdDev,curRatio,dev);
#endif
}*/
double getDoubleValue(char *line){
	char *e=0;
	double v=strtod(line,&e);
	if ((e==line)||(v==HUGE_VAL)||(v==-HUGE_VAL)){
               	LM("cannot parse %s\n",line);
               	return(-1);
	}
	return(v);
}
int initModel(){
	FILE *modelFile=fopen(SLDMODELS,"re");
	LM("model file is %s",SLDMODELS);
	logMessage(stderr,__FUNCTION__,__LINE__,"model file is %s",SLDMODELS);
	if (modelFile==0){
		LM("fopen(%s) failed with %s\n",SLDMODELS,strerror(errno));
		return(-1);
	}
	char line[LONGSTRING];
	while (fgets(line,LONGSTRING,modelFile)){
		if (strncmp(line,"TIME03: ",sizeof("TIME03: ")-1)==0){
			TIME03=getDoubleValue(&line[sizeof("TIME03: ")-1]);
		}
		else if (strncmp(line,"TIME100: ",sizeof("TIME100: ")-1)==0){
			TIME100=getDoubleValue(&line[sizeof("TIME100: ")-1]);
		}
		else if (strncmp(line,"TIME97: ",sizeof("TIME97: ")-1)==0){
			TIME97=getDoubleValue(&line[sizeof("TIME97: ")-1]);
		}
		else if (strncmp(line,"TIME0: ",sizeof("TIME0: ")-1)==0){
			TIME0=getDoubleValue(&line[sizeof("TIME0: ")-1]);
		}
		else if (strncmp(line,"COUNT03: ",sizeof("COUNT03: ")-1)==0){
			COUNT03=getDoubleValue(&line[sizeof("COUNT03: ")-1]);
		}
		else if (strncmp(line,"COUNT100: ",sizeof("COUNT100: ")-1)==0){
			COUNT100=getDoubleValue(&line[sizeof("COUNT100: ")-1]);
		}
		else if (strncmp(line,"COUNT97: ",sizeof("COUNT97: ")-1)==0){
			COUNT97=getDoubleValue(&line[sizeof("COUNT97: ")-1]);
		}
		else if (strncmp(line,"COUNT0: ",sizeof("COUNT0: ")-1)==0){
			COUNT0=getDoubleValue(&line[sizeof("COUNT0: ")-1]);
		}
	}
	if (TIME03<=0){
		LM("No TIME03 in %s",SLDMODELS);
		exit(-1);
	}
	if (TIME97<=0){
		LM("No TIME97 in %s",SLDMODELS);
		exit(-1);
	}
	if (TIME0<=0){
		LM("No TIME0 in %s",SLDMODELS);
		exit(-1);
	}
	if (TIME100<=0){
		LM("No TIME100 in %s",SLDMODELS);
		exit(-1);
	}
	if (COUNT03<=0){
		LM("No COUNT03 in %s",SLDMODELS);
		exit(-1);
	}
	if (COUNT97<=0){
		LM("No COUNT97 in %s",SLDMODELS);
		exit(-1);
	}
	if (COUNT0<=0){
		LM("No COUNT0 in %s",SLDMODELS);
		exit(-1);
	}
	if (COUNT100<=0){
		LM("No COUNT100 in %s",SLDMODELS);
		exit(-1);
	}
	fclose(modelFile);
	return(0);
}
