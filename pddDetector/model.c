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
#include "../common/flags.h"
#include "defs.h"
#include "pddFileDefs.h"
#include "pdd_seq_detector.h"
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
scModel *TCPMODEL[MAXSCVEC+1];
scModel *TCPSTARTMODEL[MAXSCVEC+1];
static char MODELERROR[ERROR_STRING];
void freeModels(){
	for(size_t i=0;i<MAXSCVEC+1;++i){
		if (TCPMODEL[i] != 0){free(TCPMODEL[i]);}
		if (TCPSTARTMODEL[i] != 0){free(TCPSTARTMODEL[i]);}
	}
}
scVector* scVectorAt(scVector *v0,int size,int pos){
	unsigned char *v=(unsigned char *)v0;
	v = v+(sizeof(scVector)+sizeof(int)*size)*pos;
	return((scVector *)v);
}
char *doMFLine(char *line,int n,scVector *v){
	int i=0;
	char *t=strstr(line," \n");
	if (t!=0){*t='\0';}
	t=strtok(line," ");
	while (t != 0){
		v->sc[i++] = atoi(t);
		t=strtok(0," ");
	}
	if (i != n) {
		sprintf(MODELERROR," %s.%d got %d of %d",
				__FUNCTION__,__LINE__,i,n);
		return(MODELERROR);
	}
	return(0);
}
char *initModel(char *modelName,scModel *m[]){
	for (int i=0;i<=MAXSCVEC;++i){
		char line[LONGSTRING];
		int nlines=0;
		char modelFileName[PATH_MAX];
		snprintf(modelFileName,PATH_MAX,"%s.%d",modelName,i);
		FILE *f=fopen(modelFileName,"re");
		if (f==0){
			if (errno != ENOENT){
				sprintf(MODELERROR,"%s.%d fopen %s %s",
					__FUNCTION__,__LINE__,
					modelFileName,strerror(errno));
				return(MODELERROR);
			}
			m[i]=0;
			continue;
		}
		nlines=0;
		while(fgets(line,sizeof(line),f)){++nlines;}
		if ((m[i]=malloc(sizeof(scModel)
				 + nlines * (sizeof(scVector)+sizeof(int)*i)))==0){
			sprintf(MODELERROR,"%s.%d malloc",__FUNCTION__,__LINE__);
			fclose(f);
			return(MODELERROR);
		}
		m[i]->n=nlines;
		rewind(f);
		nlines=0;
		while(fgets(line,sizeof(line),f)){
			scVector *v = scVectorAt(&m[i]->v[0],i,nlines);
			char *e=doMFLine(line,i, v);
			if (e!=0){
				fclose(f);
				free(m[i]);
				m[i]=0;
				return(e);
			}
			++nlines;
		}
		fclose(f);
	}
	return(0);
}
char *initModels(){
	char *e=initCwMap();
 	if (e!=0){
		return(e);
	}
	initFlagToCW();
	if ((e=initModel(TCPMODELS,TCPMODEL))!=0){
		return(e);
	}
	if ((e=initModel(TCPSTARTMODELS,TCPSTARTMODEL))!=0){
		return(e);
	}
	return(0);
}
