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
#include "pddFileDefs.h"
#include <errno.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#define DIRMODE 0777
int mkdirEach(char *path){
	size_t n=strlen(path);
	char part[PATH_MAX];
	size_t k=0;
	for(size_t i=0;i<n;++i){
		if ((part[k++]=path[i])=='/'){
			part[k]='\0';
			if (mkdir(part,DIRMODE)!=0){
				if (errno != EEXIST){
					fprintf(stderr,
						"%s.%d mkdir(%s):%s of %s",
						__FUNCTION__,__LINE__,
						part,path,strerror(errno));
		       			return(-1);
				}
			}
		}
	}
	part[k]='\0';
	if (mkdir(part,DIRMODE)!=0){
		if (errno != EEXIST){
			fprintf(stderr,"%s.%d mkdir(%s): %s of %s",
				__FUNCTION__,__LINE__,part,path,
				strerror(errno));
		       	return(-1);
		}
	}
	return(0);
}
int main(){
	FILE *f=0,*t=0;
	char lastFileName[PATH_MAX],fileName[PATH_MAX];
	char buf[PATH_MAX+LONGSTRING];
	char obsPath[PATH_MAX];
	char hostName[HOST_NAME_MAX];
	size_t totLines=0;
	if (gethostname(hostName,HOST_NAME_MAX)==-1){
		fprintf(stderr,"gethostname() %s",strerror(errno));
		strncpy(hostName,"UNKNOWNHOST",HOST_NAME_MAX);
	}
	fprintf(stdout,"cat and sorting\n");
	snprintf(buf,sizeof(buf),"for i in %s.thread*; do cat $i; done > %s.cat",MASTERTRAINFILE,MASTERTRAINFILE);
	system(buf);
	snprintf(buf,sizeof(buf),"sort -u %s.cat > %s.sorted",MASTERTRAINFILE,MASTERTRAINFILE);
	system(buf);
	snprintf(buf,sizeof(buf),"cp %s.sorted %s",MASTERTRAINFILE,MASTERTRAINFILE);
	system(buf);
	if ((f=fopen(MASTERTRAINFILE,"re"))==0){
		fprintf(stderr,
			"fopen(%s): %s\n",
			MASTERTRAINFILE,strerror(errno));
		exit(-1);
	}
	snprintf(obsPath,sizeof(obsPath),OBSDIR,hostName,time(0));
	mkdirEach(obsPath);
	if (chdir(obsPath)!=0){
		fprintf(stderr,"chdir(%s): %s\n",obsPath,strerror(errno));
		exit(-1);
	}
	lastFileName[0]='\0';
	while(fgets(buf,sizeof(buf),f)){
		++totLines;
	}
	rewind(f);
	fprintf(stdout,"processing %lu lines\n",totLines);
	while(fgets(buf,sizeof(buf),f)){
		if (sscanf(buf,"TRAIN %s ",fileName)!=1){
			fprintf(stderr,"%s.%d cannot parse %s\n",__FUNCTION__,__LINE__,buf);
			exit(-1);
		}
		if (strncmp(fileName,lastFileName,PATH_MAX)!=0){
			if (t != 0){fclose(t);}
			if ((t=fopen(fileName,"a+e"))==0){
				fprintf(stderr,"%s.%d fopen(%s): %s",
				__FUNCTION__,__LINE__,fileName,
				strerror(errno));
				exit(-1);
			}
			strncpy(lastFileName,fileName,PATH_MAX);
		}
		fprintf(t,"%s",&buf[strlen("TRAIN ")+strlen(fileName)+1]);
	}
	fclose(f);
}
