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
#include "defs.h"
#include "prdModel.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
int isPRD(const struct dirent *d){

	if (strncmp(d->d_name,"prdObs.",7)==0){
		return(1);
	}
	return(0);
}
int prdCount[MAXCW*2];
prdModel PRDMODEL;
void addCwCount(long long int cw,long long int count){
	prdCount[cw+MAXCW]+=count;
	if ((cw+MAXCW)<PRDMODEL.low){
		PRDMODEL.low=cw+MAXCW;
	}
	if ((cw+MAXCW)>PRDMODEL.high){
		PRDMODEL.high=cw+MAXCW;
	}
}
int createModel(){
	int i,j,n;
	FILE *modelFile;
	char cmd[512];
	if (mkdir(MODELDIR,777)!=0){
		if (errno != EEXIST){
                	fprintf(stderr,"%s.%d mkdir(%s) failed with %s\n",
                        	__FUNCTION__,__LINE__,MODELDIR,strerror(errno));
                	return(-1);
		}
	}

        if ((modelFile=fopen(MODELFILE,"w"))==0){
                fprintf(stderr,"%s.%d fopen(%s) failed with %s\n",
                        __FUNCTION__,__LINE__,MODELFILE,strerror(errno));
                return(-1);
	}
	for(i=0;i<MAXCW;++i) for(j=0;j<MAXCW;++j) PRDMODEL.ratio[i][j]=-1;
	for(i=PRDMODEL.low;i<PRDMODEL.high;++i){
		for(j=PRDMODEL.low;j<PRDMODEL.high;++j){
			if ((prdCount[i]!=0) && (prdCount[j]!=0)){
				PRDMODEL.ratio[i][j] =
					(int)(((double)prdCount[i]/(double)prdCount[j])*100);
			} 
			else if ((i==0) && (j==0)) PRDMODEL.ratio[i][j]=1;
			else PRDMODEL.ratio[i][j]=0;
		}
	}
	if (fwrite(&PRDMODEL,sizeof(prdModel),1,modelFile) != 1){
                fprintf(stderr,"%s.%d fwrite() %s\n",
                        __FUNCTION__,__LINE__,strerror(errno));
	}
	fclose(modelFile);
}

int processInFile(char *inDirName,char *inFileName){
        FILE *f;
	int i;
	long long int count,cw;
        char b[512];
        sprintf(b,"%s/%s",inDirName,inFileName);
        if ((f=fopen(b,"r"))==0){
                fprintf(stderr,"%s.%d fopen(%s) failed with %s\n",
                        __FUNCTION__,__LINE__,b,strerror(errno));
                return(-1);
        }
	while (fgets(b,512,f)!=0){
		if (sscanf(b,"%lld %lld\n",&cw,&count)!=2){
                	fprintf(stderr,"%s.%d parse error: %s\n",
                        	__FUNCTION__,__LINE__,b);
                	return(-1);
		}
		addCwCount(cw,count);
	}
        fclose(f);
        return(0);
}
int processInDir(char *inDirName){
	struct dirent **file;
	int i,nFiles;
	if ((nFiles=scandir(inDirName,&file,isPRD,0))<0){
		fprintf(stderr,"%s.%d scandir failed with %s\n",
			__FUNCTION__,__LINE__,strerror(errno));
		return(-1);
	}
	for(i=0;i<nFiles;++i){
		printf("%d of %d\n",i,nFiles);
		processInFile(inDirName,file[i]->d_name);
		free(file[i]);
	}
	return(0);
}

int main(){
	PRDMODEL.low=2*MAXCW;
	PRDMODEL.high=0;
	processInDir("/usr/lookout/observations/PRD");
	createModel();
}

