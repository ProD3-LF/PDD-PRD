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
#include "../common/config.h"
#include "../common/decay.h"
#include "../common/logMessage.h"
#include "alertRate.h"
#include "ar.h"
#include "baseProg.h"
#include "classifier.h"
#include "correlatorFileDefs.h"
#include "formula.h"
#include "networkEvidence.h"
#include "scenario.h"

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <zmq.h>
int RECORD=0,DETECT=1;
extern char MYHOSTNAME[];
void configureCorrelator(FILE *configFile){
	char b[LONGSTRING];
	strncpy(ARMODELFILE,ARMODELFILE_DEFAULT,PATH_MAX-1);
	sprintf(b,"correlator%s,mode",MYHOSTNAME);
	char *t=get_config_string(configFile,b);
	if (t==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
			"no host specific mode in config %s\n",b);
		if ((t=get_config_string(configFile,
			"correlator,mode"))==0){
	    		logMessage(stderr,__FUNCTION__,__LINE__,
				"no host independent mode in config\n");
		}
	}
	if (t==0){
		RECORD=false;
		DETECT=false;
		logMessage(stderr,__FUNCTION__,__LINE__,
			"no mode in config default to nothing\n");
	}
	else if (strcmp(t,"donothing")==0){
		RECORD=false;
		DETECT=false;
		logMessage(stderr,__FUNCTION__,__LINE__,
				"mode is donothing\n");
		free(t);
	} else if (strcmp(t,"record")==0){
		RECORD=true;
		DETECT=false;
		logMessage(stderr,__FUNCTION__,__LINE__,
				"mode is record\n");
		free(t);
	} else if (strcmp(t,"detect")==0){
		DETECT=true;
		RECORD=false;
		logMessage(stderr,__FUNCTION__,__LINE__,
				"mode is detect\n");
		free(t);
	} else if (strcmp(t,"detect-record")==0){
		RECORD=true;
		DETECT=true;
	 	logMessage(stderr,__FUNCTION__,__LINE__,
				"mode is detect-record\n");
	 	free(t);
	} else {
		logMessage(stderr,__FUNCTION__,__LINE__,
				"ERROR bad mode: %s\n",t);
		RECORD=false;
		DETECT=false;
		free(t);
	}
	char *scenario=get_config_string(configFile,
			"correlator,genericScenarioFile");
	if (scenario!=0){
		strncpy(GENERIC_SCENARIO_FILE,scenario,PATH_MAX-1);
	} else {
		logMessage(stderr,__FUNCTION__,__LINE__,
			"no genericScenarioFile in correlator.ini\n");
	}
	char *formula=get_config_string(configFile,
			"correlator,genericFormulaFile");
	if (formula!=0){
		strncpy(GENERIC_FORMULA_FILE,formula,PATH_MAX-1);
	} else {
		logMessage(stderr,__FUNCTION__,__LINE__,
			"no genericFormulaFile in correlator.ini\n");
	}
	char *model=get_config_string(configFile,"correlator,modelFile");
	if (model!=0){
		strncpy(ARMODELFILE,model,PATH_MAX-1);
	} else {
		logMessage(stderr,__FUNCTION__,__LINE__,
				"no modelFile in correlator.ini\n");
	}
	if ((t=get_config_string(configFile,
				"correlator,RATETHRESHOLD"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"no RATETHRESHOLD in correlator.ini\n");
		RATETHRESHOLD=DEFAULTTHRESHOLD;
	} else {
		RATETHRESHOLD=strToPI(t);
	}
	if ((t=get_config_string(configFile,
				"correlator,AGETHRESHOLD"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"no AGETHRESHOLD in correlator.ini\n");
		AGETHRESHOLD=AGEFACTOR;
	} else {
		AGETHRESHOLD=strToPI(t);
	}
	for(size_t i=0;i<MAXCLASSRULES;++i){
      		sprintf(b,"correlator,classRule%ld",i);
      		if ((t=get_config_string(configFile,b))!=0){
	      		addClassRule(t);
		}
	}
	for(size_t i=0;i<MAXATTRIBUTIONRULES;++i){
      		sprintf(b,"correlator,attributionRule%ld",i);
      		if ((t=get_config_string(configFile,b))!=0){
	      		addAttributionRule(t);
		}
	}
	dumpClassRules();
	dumpAttributionRules();
}
