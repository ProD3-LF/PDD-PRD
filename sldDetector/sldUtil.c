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
#include "../common/logMessage.h"
#include "../common/util.h"
#include "defs.h"
#include "sldAlert.h"
#include "sldDetector.h"
#include "sldFileDefs.h"
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
FILE *ssvFile;
FILE *gFile;
FILE *obsFile=0;
char hostName[HOST_NAME_MAX];
void setupObsFile(){
	char obsFilePath[PATH_MAX];
	char obsFileName[PATH_MAX];
	snprintf(obsFilePath,PATH_MAX,OBSDIR,hostName,startTime);
	if (mkdirEach(obsFilePath)!=0){
		logMessage(stderr,__FUNCTION__,__LINE__,"mkdirEach(%s): %s",obsFilePath,strerror(errno));
		return;
	}
	snprintf(obsFileName,PATH_MAX,SLDOBSFILE,hostName,startTime,startTime);
	obsFile=fopen(obsFileName,"w+e");
	if (obsFile==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"fopen(%s): %s\n",
			obsFileName,strerror(errno));
	}
}
uint16_t strToU16(char *s){
	char *e=0;
	long int t=strtol(s,&e,0);
	if ((t==LONG_MIN) || (t==LONG_MAX) || (e==s)){
		logMessage(stderr,__FUNCTION__,__LINE__,
			"strtol cannot convert %s\n",s);
		t=0;
	}
	if ((t < 0) || (t > USHRT_MAX)){
		logMessage(stderr,__FUNCTION__,__LINE__,
			"out of ushort range %ld\n",t);
		t=0;
	}
	return(t);
}
void setupServers(char *s){
	size_t n=strlen(s);
	char *t=s;
	for(size_t i=0;i<n;++i){
		if (s[i]==':'){
			size_t j=0;
			s[i]='\0';
			addKnownServer(strToU16(&s[i+1]),inet_addr(t));
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
void sldConfig(){
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
  sprintf(b,"sld%s,mode",hostName);
  char *t=get_config_string(configFile,b);
  if (t==0){
     logMessage(stderr,__FUNCTION__,__LINE__,"no host specific mode in config %s",b);
     if ((t=get_config_string(configFile,"sldmode"))==0){
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
  if (adRecord == true){
	  setupObsFile();
  }
  sprintf(b,"sld%s,servers",hostName);
  if ((t=get_config_string(configFile,b))==0){
      logMessage(stderr,__FUNCTION__,__LINE__,
		      "no host specific servers in config %s",b);
        if ((t=get_config_string(configFile,"sldservers"))==0){
                logMessage(stderr,__FUNCTION__,__LINE__,
			"no host independent servers in config");
		exit(-1);
        }
  }
  setupServers(t);
  free(t);
}
void sldInit() {
	sldConfig();
	initSERVD();
	(*INITALERT)();
	if (initModel()!=0){
		logMessage(stderr,__FUNCTION__,__LINE__,"initModels failed\n");
		exit(-1);
	}
	if ((ssvFile=fopen("/tmp/SLD.csv","w+e"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"fopen %s %s\n","/tmp/SLD.csv",strerror(errno));
		ssvFile=stderr;
	}
	if ((gFile=fopen("/tmp/SLDG","w+e"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"fopen %s %s","/tmp/SLDG",strerror(errno));
		exit(-1);
	}
	fprintf(ssvFile,"time,obstime");
	for (size_t i=0;i<SLD_ANOMMAX;++i){
		fprintf(ssvFile,",%s",sldTimeAlertToName(i));
	}
	for (size_t i=0;i<SLD_ANOMMAX;++i){
		fprintf(ssvFile,",%s",sldCountAlertToName(i));
	}
	fprintf(ssvFile,"\n");
	fflush(ssvFile);
}
