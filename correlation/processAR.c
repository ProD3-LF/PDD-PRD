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
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
alertsRate *FIRST=0;
int isAR(const struct dirent *d){
	if (strncmp(d->d_name,"AR.alertsRate",sizeof("AR.alertsRate")-1)==0){
		return(1);
	}
	return(0);
}
void outputModel(){
	char fileName[PATH_MAX];
	char oldFileName[PATH_MAX];
	alertsRate *f = FIRST;
	sprintf(fileName,"%s/ar",ARMODELDIR);
	sprintf(oldFileName,"%s/ar.%ld",ARMODELDIR,time(0));
	rename(fileName,oldFileName);
	FILE *o=fopen(fileName,"we");
	if (o==0){
		fprintf(stderr,"%s.%d ERROR fopen %s\n",
			__FUNCTION__,__LINE__,strerror(errno));
	}
	while (f != 0){
		fprintf(o,"%s	%d	%d\n",
			f->appName, f->alertType, f->score);
		f = f->next;
	}
	fclose(o);
}
void addScore(char *appName,int alertType, int score){
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
		return;
	}
	alertsRate *f = FIRST;
	while(f != 0) {
		if (strncmp(f->appName,appName,LONGSTRING)==0){
			if (f->alertType == alertType) {
				if(f->old == 1) {
					f->score = score;
					f->old=0;
					return;
				}
				f->old=0;
				if (f->score < score){f->score=score;}
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
			f->old=0;
			f->next=0;
			strncpy(f->appName,appName,LONGSTRING);
			f->alertType=alertType;
			f->score=score;
			return;
		}
		f=f->next;
	}
	fprintf(stderr,"%s.%d ERROR cannot get here\n",__FUNCTION__,__LINE__);
}

int processInFile(char *inDirName,char *inFileName){
	char b[LONGSTRING];
	char appName[LONGSTRING];
	int alertType=0;
	int score=0;
	char cmd[LONGSTRING];
	sprintf(cmd,"/usr/bin/sort -u %s/%s -o /tmp/tmpAR",inDirName,inFileName);
	system(cmd); //NOLINT(cert-env33-c)
	sprintf(b,"/tmp/tmpAR");
	FILE *f=fopen(b,"re");
	if (f==0){
		fprintf(stderr,"%s.%d fopen(%s) failed %s\n",
			__FUNCTION__,__LINE__,b,strerror(errno));
		fclose(f);
		system("/bin/rm /tmp/tmpAR"); //NOLINT(cert-env33-c)
		return(-1);
	}
	while(fgets(b,LONGSTRING,f)!=0){
		if (parseLine(b,LONGSTRING,appName,&alertType,&score)==0){
			addScore(appName,alertType,score);
		}
		else {
			fprintf(stderr,"%s.%d ERROR cannot scan %s\n",
				__FUNCTION__,__LINE__,b);
		}
	}
	system("/bin/rm /tmp/tmpAR"); //NOLINT(cert-env33-c)
	fclose(f);
	return(0);
}
#define DIRMODE 0777
int processInDir(char *inDirName){
	struct dirent **file=0;
	int nFiles=scandir(inDirName,&file,isAR,0);
	if (nFiles<0){
		fprintf(stderr,"%s.%d scandir failed %s\n",
			__FUNCTION__,__LINE__,strerror(errno));
		return(-1);
	}
	int i=0;
	for(;i<nFiles;++i){
		printf("%d of %d\r",i,nFiles);
		fflush(stdout);
		processInFile(inDirName,file[i]->d_name);
		free(file[i]);
	}
	printf("processed %d observation files\n",i);
	outputModel();
	return(0);
}
int mkdirEach(char *path,int flags){
	size_t n=strlen(path);
	char part[PATH_MAX];
	size_t k=0;
	for(size_t i=0;i<n;++i){
		if ((part[k++]=path[i])=='/'){
			part[k]='\0';
			if (mkdir(part,flags)!=0){
				if (errno != EEXIST){
					fprintf(stderr,"mkdir(%s):%s of %s",
						part,path,strerror(errno));
		       			return(-1);
				}
			}
		}
	}
	part[k]='\0';
	if (mkdir(part,flags)!=0){
		if (errno != EEXIST){
			fprintf(stderr,"mkdir(%s): %s of %s", part,path,
				strerror(errno));
		       	return(-1);
		}
	}
	return(0);
}
int main(int argc, char *argv[]){
	char inDirName[PATH_MAX];
	if (argc != 3){
		fprintf(stderr,"ERROR %s.%d usage processAR -i dir\n",
				__FUNCTION__,__LINE__);
		exit(-1);
	}
	if (mkdirEach(ARMODELDIR,DIRMODE)!=0){
		if (errno != EEXIST){
			fprintf(stderr,"ERROR %s.%d mkdir(%s) failed %s\n",
				__FUNCTION__,__LINE__,ARMODELDIR,strerror(errno));
			exit(-1);
		}
	}
	int i=1;
fprintf(stderr,"%s.%d\n",__FUNCTION__,__LINE__);fflush(stderr);
	inDirName[0]=0;
	loadCurrentAR();
fprintf(stderr,"%s.%d\n",__FUNCTION__,__LINE__);fflush(stderr);
	while(i<argc){
		if (strncmp(argv[i],"-i",sizeof("-1")-1)==0){
			++i;
			strncpy(inDirName,argv[i],PATH_MAX);
			++i;
			continue;
		}
		fprintf(stderr,"ERROR %s.%d usage bad arg %s\n",__FUNCTION__,__LINE__,argv[i]);
		exit(-1);
	}
	if (inDirName[0]==0){
		fprintf(stderr,"ERROR %s.%d no inDirName specified\n",__FUNCTION__,__LINE__);
		exit(-1);
	}
fprintf(stderr,"%s.%d\n",__FUNCTION__,__LINE__);fflush(stderr);
	processInDir(inDirName);
fprintf(stderr,"%s.%d\n",__FUNCTION__,__LINE__);fflush(stderr);
	exit(0);
}
