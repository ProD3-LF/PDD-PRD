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
#include "../common/config.h"
#include "../common/logMessage.h"
#include "../common/prdObs.h"
#include "defs.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
bool adDetect=false,adRecord=false;
void setupServers(char *s){
        size_t n=strlen(s);
        char *t=s;
        for(size_t i=0;i<n;++i){
                if (s[i]==':'){
                        s[i]='\0';
        		in_addr_t ip=inet_addr(t);
        		uint16_t port=atoi(&s[i+1]);
                        initSD(ip,port);
                        s[i]=':';
                	size_t j=i+1;
                        for(;j<n;++j){
                                if (s[j] == ' '){break;}
                        }
                        i=j;
                        t=&s[j+1];
                }
        }
        logMessage(stderr,__FUNCTION__,__LINE__,"Servers are set up");
}
void prdConfig(){
  FILE *configFile=fopen(CONFIGFILE,"r+e");
  if (configFile==0){
         logMessage(stderr,__FUNCTION__,__LINE__,"cannot open %s",
                         CONFIGFILE);
         exit(-1);
  }
  if (gethostname(hostName,HOST_NAME_MAX)==-1){
	logMessage(stderr,__FUNCTION__,__LINE__,"gethostname() %s\n",strerror(errno));
	strncpy(hostName,"UNKNOWNHOST",HOST_NAME_MAX);
  }
  adRecord=adDetect=false;
  char b[LONGSTRING];
  sprintf(b,"prd%s,mode",hostName);
  char *t=get_config_string(configFile,b);
  if (t==0){
     logMessage(stderr,__FUNCTION__,__LINE__,"no host specific mode in config %s",b);
     if ((t=get_config_string(configFile,"prdmode"))==0){
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
	  free(t);
  } else if (strcmp(t,"record")==0){
	  adRecord=true;
	  adDetect=false;
	  logMessage(stderr,__FUNCTION__,__LINE__,"mode is record");
	  free(t);
  } else if (strcmp(t,"detect")==0){
	  adDetect=true;
	  adRecord=false;
	  logMessage(stderr,__FUNCTION__,__LINE__,"mode is detect");
	  free(t);
  } else if (strcmp(t,"detect-record")==0){
	  adRecord=true;
	  adDetect=true;
	  logMessage(stderr,__FUNCTION__,__LINE__,"mode is detect-record");
	  free(t);
  } else {
	  logMessage(stderr,__FUNCTION__,__LINE__,"ERROR bad mode: %s\n",t);
	  adRecord=false;
	  adDetect=false;
	  free(t);
  }
  sprintf(b,"prd%s,servers",hostName);
  if ((t=get_config_string(configFile,b))==0){
      logMessage(stderr,__FUNCTION__,__LINE__,
		      "no host specific servers in config %s",b);
        if ((t=get_config_string(configFile,"prdservers"))==0){
                logMessage(stderr,__FUNCTION__,__LINE__,
			"no host independent servers in config");
		exit(-1);
        }
  }
  setupServers(t);
  free(t);
}
void prdInit() {
	(*INITALERT)();
	prdConfig();
	initSERVD();
	if (initModels()!=0){
		logMessage(stderr,__FUNCTION__,__LINE__,"initModels failed\n");
		exit(-1);
	}
	if ((ssvFile=fopen("/tmp/PRD.ssv","w+e"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"fopen %s %s\n","/tmp/PRD.ssv",strerror(errno));
		ssvFile=stderr;
	}
	if ((gFile=fopen("/tmp/PRDG","w+e"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"fopen %s %s","/tmp/PDDG",strerror(errno));
		exit(-1);
	}
	fprintf(ssvFile,"time,allmax,alltwo,max,two,deviant,zero,error\n");
	fflush(ssvFile);
	initCounts();
}
