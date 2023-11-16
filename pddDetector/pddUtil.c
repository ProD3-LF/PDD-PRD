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
#include "../common/config.h"
#include "../common/config.h"
#include "../common/logMessage.h"
#include "../common/util.h"
#include "defs.h"
#include "pdd_seq_detector.h"
#include "pddFileDefs.h"
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
extern time_t startTime;
unsigned int statsdFlushInterval = 4;
bool adDetect=false,adRecord=false;
void setupServers(char *s){
	size_t n=strlen(s);
	char *t=s;
	for(size_t i=0;i<n;++i){
		if (s[i]==':'){
			size_t j=0;
			s[i]='\0';
			addKnownServer(atoi(&s[i+1]),inet_addr(t));
			s[i]=':';
			for(j=i+1;j<n;++j){
				if (s[j] == ' '){break;}
			}
			i=j;
			t=&s[j+1];
		}
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"Servers are set up");
}
void setupObsPath(){
	char obsFilePath[PATH_MAX];
	snprintf(obsFilePath,PATH_MAX,OBSDIR,hostName,startTime);
	if (mkdirEach(obsFilePath)==0) return;
	logMessage(stderr,__FUNCTION__,__LINE__,"mkdirEach(%s): %s",obsFilePath,strerror(errno));
}
extern long long unsigned obsInterval;
void pddConfig(){
  FILE *configFile=0;
  if ((configFile=fopen(CONFIGFILE,"r+e"))==0){
	logMessage(stderr,__FUNCTION__,__LINE__,"%s.%d cannot open %s",
		CONFIGFILE);
	exit(-1);
  }
  if (gethostname(hostName,HOST_NAME_MAX)==-1){
	logMessage(stderr,__FUNCTION__,__LINE__,"gethostname() %s",strerror(errno));
	strncpy(hostName,"UNKNOWNHOST",HOST_NAME_MAX);
  }
  char *t=0;
  adRecord=adDetect=false;
  char b[LONGSTRING];
  snprintf(b,sizeof(b),"pdd%s,mode",hostName);
  if ((t=get_config_string(configFile,b))==0){
      logMessage(stderr,__FUNCTION__,__LINE__,"no host specific mode in config %s",b);
      if ((t=get_config_string(configFile,"pddmode"))==0){
      	logMessage(stderr,__FUNCTION__,__LINE__,"no host independent mode in config");
      }
  }
  if (t==0){
	adRecord=false;
	adDetect=false;
	logMessage(stderr,__FUNCTION__,__LINE__,"no mode in config default to nothing");
  }
  else if (strcmp(t,"donothing")==0){
	  adRecord=false;
	  adDetect=false;
	  logMessage(stderr,__FUNCTION__,__LINE__,"mode is donothing");
  } else if (strcmp(t,"record")==0){
	  adRecord=true;
	  adDetect=false;
	  logMessage(stderr,__FUNCTION__,__LINE__,"mode is record");
  } else if (strcmp(t,"detect")==0){
	  adRecord=0;
	  adDetect=true;
	  adRecord=false;
	  logMessage(stderr,__FUNCTION__,__LINE__,"mode is detect");
  } else if (strcmp(t,"detect-record")==0){
	  adRecord=true;
	  adDetect=true;
	  logMessage(stderr,__FUNCTION__,__LINE__,"mode is detect-record");
  } else {
	  logMessage(stderr,__FUNCTION__,__LINE__,"ERROR bad mode: %s",t);
	  adRecord=false;
	  adDetect=false;
  }
  if (t!=0){free(t);}
  snprintf(b,sizeof(b),"pdd%s,servers",hostName);
  t=0;
  if ((t=get_config_string(configFile,b))==0){
      logMessage(stderr,__FUNCTION__,__LINE__,"no host specific servers in config %s",b);
  	if ((t=get_config_string(configFile,"pddservers"))==0){
      		logMessage(stderr,__FUNCTION__,__LINE__,"no host independent servers in config: will autodetect only");
	}
  }
  if (adRecord == 1){
       	setupObsPath();
	setupThreadObsFd();
  }
  if (t != 0){
	  setupServers(t);
  	  free(t);
  }
  fclose(configFile);
}
void pddInit() {
  	(*INITALERT)();
	setlocale(LC_NUMERIC, "");
	pddConfig();
	initSERVD();
	for(size_t j=0;j<NFIFOS;++j){
		SAMPLERATE[j]=-1;
		lastSample[j]=-1;
	}
	for(size_t j=0;j<MAXPORTS;++j){
		sampleChange[j]=0;
	}
	char *e=initModels();
	if (e!=0){
		logMessage(stderr,__FUNCTION__,__LINE__,"initModel failed %s",e);
	}
	if ((ssvFile=fopen("/tmp/PDD.ssv","w+e"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"fopen %s %s","/tmp/PDD.ssv",strerror(errno));
		ssvFile=stderr;
	}
	if ((gFile=fopen("/tmp/PDDG","w+e"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"fopen %s %s","/tmp/PDDG",strerror(errno));
		exit(-1);
	}
	fprintf(ssvFile,"time CWIN NGRAMSCHECKED NGRAMSOK NGRAMSANOM NGRAMSALIEN NGRAMSSTARTANOM NGRAMSSTARTALIEN NGRAMSSTARTSHORT NGRAMSABANDONED\n");
	fflush(ssvFile);
}
