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
#include "ar.h"
#include "correlatorFileDefs.h"
#include "defs.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
char ARMODELFILE[PATH_MAX]=ARMODELFILE_DEFAULT;
void loadScore(char *appName,int alertType, int score){
	if (FIRST==0){
		if ((FIRST=(alertsRate *)malloc(sizeof(alertsRate)))==0) {
			fprintf(stderr,"%s.%d ERROR malloc failed\n",
				__FUNCTION__,__LINE__);
			return;
		}
		FIRST->next=0;
		strncpy(FIRST->appName,appName,LONGSTRING);
		FIRST->alertType=alertType;
		FIRST->score=score;
		FIRST->old=1;
		return;
	}
	alertsRate *f = FIRST;
	while(f != 0) {
		if (strncmp(f->appName,appName,LONGSTRING)==0){
			if (f->alertType == alertType) {
				fprintf(stderr,
					"%s.%d ERROR duplicate entry\n",
				__FUNCTION__,__LINE__);
				return;
			}
		}
		if (f->next == 0){
			if ((f->next=(alertsRate *)malloc(sizeof(alertsRate)))==0) {
				fprintf(stderr,"%s.%d ERROR malloc failed\n",
				__FUNCTION__,__LINE__);
				return;
			}
			f=f->next;
			f->next=0;
			strncpy(f->appName,appName,LONGSTRING);
			f->alertType=alertType;
			f->score=score;
			f->old=1;
			return;
		}
		f=f->next;
	}
	fprintf(stderr,"%s.%d ERROR cannot get here\n",__FUNCTION__,__LINE__);
}
void loadCurrentAR(){
	char b[MEDIUMSTRING];
	fprintf(stderr,"%s.%d armodel is %s\n",
		__FUNCTION__,__LINE__,ARMODELFILE);
	FILE *o=fopen(ARMODELFILE,"re");
	if (o==0){
		fprintf(stderr,"%s.%d ERROR fopen(%s) %s\n",
			__FUNCTION__,__LINE__,ARMODELFILE,strerror(errno));
		if (errno == ENOENT){return;}
		return;
	}
	while(fgets(b,LONGSTRING,o)!=0){
		char appName[MEDIUMSTRING];
		int alertType=0;
		int score=0;
		if (parseLine(b,LONGSTRING,appName,&alertType,&score)==0){
			loadScore(appName,alertType,score);
		}
		else {
			fprintf(stderr,"%s.%d ERROR cannot scan %s\n",
				__FUNCTION__,__LINE__,b);
		}
	}
	fclose(o);
}
