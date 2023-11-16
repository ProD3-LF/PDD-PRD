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
#include "../common/flags.h"
#include "../common/prdObs.h"
#include "defs.h"
#include "prdFileDefs.h"
#include "prdStdModel.h"
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

prdStdModel PRDSTDMODEL;
int allZeroCol(int c,int l,int h){
	for (int i=l;i<h;++i){
		if(i==c){continue;}
		 if (PRDSTDMODEL.dist[i][c].mean!=-1){return(1);}
	}
	return(0);
}
int allZeroRow(int r,int l,int h){
	for (int i=l;i<h;++i){
		if(i==r){continue;}
		 if (PRDSTDMODEL.dist[r][i].mean!=-1){return(1);}
	}
	return(0);
}
int dumpModel(int start, int stop){
	int lowC=MAXCW*2,highC=0,lowR=MAXCW*2,highR=0;
        fprintf(stderr,"file is %s\n",STOREDSTDMODELFILE);
	FILE *modelFile=fopen(STOREDSTDMODELFILE,"re");
        if (modelFile==0){
                fprintf(stderr,"%s.%d fopen(%s) failed with %s\n",
                        __FUNCTION__,__LINE__,STOREDSTDMODELFILE,strerror(errno));
                return(-1);
	}
	if (fread(&PRDSTDMODEL,sizeof(prdStdModel),1,modelFile) != 1){
                fprintf(stderr,"%s.%d fread() failed %s\n",
                        __FUNCTION__,__LINE__,strerror(errno));
                return(-1);
	}
	fclose(modelFile);
	for(int k=0;k<MAXCW*2;++k){
		for(int j=0;j<MAXCW*2;++j){
			if (PRDSTDMODEL.dist[k][j].mean!=-1){
				if (k<lowC){lowC=k;}
				if (j<lowR){lowR=j;}
				if (j>highR){highR=j;}
				if (k>highC){highC=k;}
			}
		}
	}
	++highC;
	++highR;
	for(int k=lowC;k<highC;++k){
		if (inRequestedRange(start,stop,k) == 1){continue;}
		if (allZeroCol(k,lowR,highR)==0){continue;}
				fprintf(stderr,",%s",convert(k-MAXCW));
		//fprintf(stderr,",%d",(k-MAXCW));
	}
	fprintf(stderr,",ALL");
	fprintf(stderr,"\n");
	for(int k=lowC;k<highC;++k){
		if (allZeroCol(k,lowR,highR)==0){continue;}
		if (inRequestedRange(start,stop,k) == 1){continue;}
		fprintf(stderr,"%s",convert(k-MAXCW));
		//fprintf(stderr,"%d",(k-MAXCW));
		for(int j=lowR;j<highR;++j){
			if (allZeroRow(j,lowC,highC)==0){continue;}
			if (inRequestedRange(start,stop,j) == 1){continue;}
		//	fprintf(stderr,",%d", PRDSTDMODEL.dist[k][j].mean);
				fprintf(stderr,",%d %d %d",
					PRDSTDMODEL.dist[k][j].mean,
					PRDSTDMODEL.dist[k][j].stddev,
					PRDSTDMODEL.dist[k][j].max);
		}
		fprintf(stderr,",%d:%d:%d",
				PRDSTDMODEL.allDist[k].mean,
				PRDSTDMODEL.allDist[k].stddev,
				PRDSTDMODEL.allDist[k].max);
		fprintf(stderr,"\n");
	}
	return(0);
}
int main(int argc, char *argv[]){
	int start=0,stop=MAXCW;
	if (argc == 3){
		start=atoi(argv[1]);
		stop=atoi(argv[2]);
		if (start > stop) {
			fprintf(stderr,"%s.%d stop must be > start\n",__FUNCTION__,__LINE__);
			return(-1);
		}

	}
	else if (argc != 1){
		fprintf(stderr,"%s.%d usage %s or %s stop start\n",
				__FUNCTION__,__LINE__,argv[0],argv[0]);
		return(-1);
	}
	char *e=initCwMap();
	if (e!=0){
		fprintf(stderr,"%s.%d initCwMap(): %s\n",__FUNCTION__,__LINE__,e);
		return(-1);
	}
	dumpModel(start,stop);
}

