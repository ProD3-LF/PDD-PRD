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
#include "../common/logMessage.h"
#include "defs.h"
#include "pddFileDefs.h"
#include "pddModel.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#define MAXSCVEC 20
#define VFMT " %80[^{},\n\t]"
time_t oldestModelTime(){
	time_t OLDESTMODEL=UINT_MAX;
	for (size_t i=0;i<=MAXSCVEC;++i){
		struct stat modStat;
		char modelFileName[PATH_MAX];
		snprintf(modelFileName,sizeof(modelFileName),
				"%s.%lu",TCPMODELS,i);
		if (stat(modelFileName,&modStat)!=0){continue;}
		if (modStat.st_mtime < OLDESTMODEL){
			OLDESTMODEL=modStat.st_mtime;
		}
		snprintf(modelFileName,sizeof(modelFileName),
				"%s.%lu",TCPSTARTMODELS,i);
		if (stat(modelFileName,&modStat)!=0){continue;}
		if (modStat.st_mtime < OLDESTMODEL){
			OLDESTMODEL=modStat.st_mtime;
		}
	}
	return(OLDESTMODEL);
}
static int compareVSv(const int *v1, const scVector * v2,int ng) {
	for(int i=0;i<ng;++i){
		if (v1[i] < v2->sc[i]){
			return (-1);
		}
		if (v1[i] > v2->sc[i]){
			return (1);
		}
	}
	return (0);
}
static void copySCVect(scVector *d,const scVector *s,int ng){
	for(int i=0;i<ng;++i){
		d->sc[i]=s->sc[i];
	}
}
static void copySCV(scVector *d,const int s[],int ng){
	for(int i=0;i<ng;++i){
		d->sc[i]=s[i];
	}
}

int insertNgram(int rc,int m,int v[],int ng,scModel *sd){
	if (rc < 0){
		for(int i=sd->n;i>m;--i){
			scVector *vm1 = scVectorAt(&sd->v[0],ng,i);
			scVector *vm2 = scVectorAt(&sd->v[0],ng,i-1);
			copySCVect(vm1,vm2,ng);
		}
		scVector *vm = scVectorAt(&sd->v[0],ng,m);
		copySCV(vm,v,ng);
		sd->n++;
		return(1);
	}
	for(int i=sd->n;i>m+1;--i){
		scVector *vm1 = scVectorAt(&sd->v[0],ng,i-1);
		scVector *vm2 = scVectorAt(&sd->v[0],ng,i);
		copySCVect(vm2,vm1,ng);
	}
	scVector *vm = scVectorAt(&sd->v[0],ng,m+1);
	copySCV(vm,v,ng);
	sd->n++;
	return(1);
}
int findOrAddVScModel(scModel **scp,int vn, int *vfull, int ng){
	int s=0,m=0,rc=0;
	if ((*scp=realloc(*scp,sizeof(scModel)
		 + ((*scp)->n+1) * (sizeof(scVector)+sizeof(int)*ng)))==0){
		fprintf(stderr,"%s.%d realloc\n",__FUNCTION__,__LINE__);
		exit(-1);
	}
	scModel *sc = *scp;
	int *v = &vfull[vn-ng];
	if (sc->n == 0) {
		return(insertNgram(-1,0,v,ng,sc));
	}
	int e = sc->n - 1;
	while (s <= e) {
		m = (s + e) / 2;
		scVector *vm = scVectorAt(&sc->v[0],ng,m);
		if ((rc = compareVSv(v,vm,ng)) == 0) {
			return (0);
		}
		if (rc < 0) {
			e = m - 1;
		}
		else {
			s = m + 1;
		}
	}
	return(insertNgram(rc,m,v,ng,sc));
}
int doLine(char *line,int n,scVector *v){
	int i=0;
	char *t=0;
	if ((t=strstr(line," \n"))!=0){
	      	*t='\0';
	}
	t=strtok(line," ");
	while (t != 0){
		v->sc[i++] = atoi(t);
		t=strtok(0," ");
	}
	if (i != n) {
		fprintf(stderr,"%s.%d got %d of %d\n",
				__FUNCTION__,__LINE__,i,n);
		return(-1);
	}
	return(0);
}
void printSCVector(FILE *f,scVector *v,int n){
	for(size_t i=0;i<n;++i){
		fprintf(f,"%d ",v->sc[i]);
	}
	fprintf(f,"\n");
}
int writeModel(char *modelName,scModel *m[]){
	for (size_t i=0;i<=MAXSCVEC;++i){
		FILE *f=0;
		if (m[i]==0){
		       	continue;
		}
		char modelFileName[PATH_MAX];
		snprintf(modelFileName,sizeof(modelFileName),"%s.%lu",modelName,i);
		if ((f=fopen(modelFileName,"we"))==0){
			fprintf(stderr,"%s.%d fopen %s %s\n",
				__FUNCTION__,__LINE__,
				modelFileName,strerror(errno));
			return(-1);
		}
		for(size_t k=0;k<m[i]->n;++k){
			scVector *vm = scVectorAt(&m[i]->v[0],i,k);
			printSCVector(f,vm,i);
		}
		fclose(f);
	}
	return(0);
}
int writeModels(){
        writeModel(TCPMODELS,TCPMODEL);
        writeModel(TCPSTARTMODELS,TCPSTARTMODEL);
        return(0);
}
int cwMapChanged=0;
#define TIMESTAMPLEN 10
int isTS(const struct dirent *d){
	if (strlen(d->d_name)!=TIMESTAMPLEN){
	       	return(0);
	}
	for (size_t i=0;i<TIMESTAMPLEN;++i){
		if ((d->d_name[i] < '0') || (d->d_name[i] > '9')){
			return(0);
		}
	}
        return(1);
}
int isPDD(const struct dirent *d){
	//should have .TRAIN. in name
        if (strstr(d->d_name,".TRAIN.")==0){
		if(strstr(d->d_name,".INCR.")==0){
			 return(0);
		}
		if(strstr(d->d_name,".START.")==0){
			return(0);
		}
        }
        return(1);
}
scModel **M;
int selectModel(char *f,int *n){
	char *p=0;
	if ((p=strstr(f,".INCR.")) != 0){
		p+=strlen(".INCR.");
		*n=atoi(p);
		if (TCPMODEL[*n]==0){
			fprintf(stderr,"%s.%d invalid model TCP %d\n",
					__FUNCTION__,__LINE__,*n);
			return(-1);
		}
		M=&TCPMODEL[*n];
		return(0);
	}
	if ((p=strstr(f,".START.")) != 0){
		p+=strlen(".START.");
		*n=atoi(p);
		if ((M=&TCPSTARTMODEL[*n])==0){
			fprintf(stderr,"%s.%d invalid model TCPSTART %d\n",
					__FUNCTION__,__LINE__,*n);
			return(-1);
		}
		return(0);
	}
	fprintf(stderr,"%s.%d cannot parse %s\n",
			__FUNCTION__,__LINE__,f);
	return(-1);

}
int createVector(int v[],int n,char *b){
	size_t i=0;
	char *t=b;
	v[i++]=atoi(t);
	while((t=strstr(t," "))!=0){
		if (strncmp(t," \n",2)== 0){
			break;
		}
		v[i++]=atoi(t);
		++t;
	}
	if (i != n) {
		fprintf(stderr,"%s.%d wrong len %ld not %d in %s\n",
				__FUNCTION__,__LINE__,
				i,n,b);
		return(-1);
	}
	return(0);
}


int processInFile(char *inDirName,char *inFileName){
        FILE *f=0;
        int mSize=0;
        char b[PATH_MAX];
	if (selectModel(inFileName,&mSize)==-1){
		fprintf(stderr,"%s.%d selectMode failed for %s\n",
				__FUNCTION__,__LINE__,inFileName);
		return(-1);
	}
	snprintf(b,sizeof(b),"%s/%s",inDirName,inFileName);
        if ((f=fopen(b,"re"))==0){
                fprintf(stderr,"%s.%d fopen(%s) failed with %s\n",
                        __FUNCTION__,__LINE__,b,strerror(errno));
                return(-1);
        }
        while (fgets(b,sizeof(b),f)!=0){
		int v[MAXSCVEC];
		if (createVector(v,mSize,b)!=0){
			fprintf(stderr,"%s.%d cannot parse %s from %s/%s\n",
				__FUNCTION__,__LINE__,b,inDirName,inFileName);
			return(-1);
		}
		findOrAddVScModel(M,mSize, v, mSize);
        }
        fclose(f);
        return(0);
}
void reWriteCwMap(){
	FILE *f=0;
	if ((f=fopen(CWMAPFILE,"we+"))==0){
		fprintf(stderr,"%s.%d cannot open %s for rewrite\n",
				__FUNCTION__,__LINE__,CWMAPFILE);
		return;
	}
	for(size_t i=0;i<CWMAPN;++i){
		fprintf(f,"%s,%d\n",CWMAP[i].controlWordString,CWMAP[i].cw);
	}
	fclose(f);
}
int handleCwMapUpdate(char *d){
	char obsMapFile[PATH_MAX],b[PATH_MAX];
	FILE *obsFile=0;
	snprintf(obsMapFile,PATH_MAX,"%s/cwmap.obs",d);
	if ((obsFile=fopen(obsMapFile,"re"))==0){
		if (errno == ENOENT){
			return(0);
		}
		fprintf(stderr,"%s.%d bad cwmap.obs in %s %s\n",
				__FUNCTION__,__LINE__,
				obsMapFile,strerror(errno));
		return(-1);
	}
	while(fgets(b,sizeof(b),obsFile)){
		char cwName[SHORTSTRING];
		int cw=0;
		size_t i=0;
		if (sscanf(b,VFMT "," "%d", cwName,&cw)!=2){
				fprintf(stderr,"%s.%d cannont parse %s",
					__FUNCTION__,__LINE__,b);
				fclose(obsFile);
				return(-1);
		}
		for(i=0;i<CWMAPN;++i){
			if (CWMAP[i].cw == cw){
				//THIS IS BAD
				fprintf(stderr,"%s.%d PROBLEM mapfile in %s cw conflict %d\n",
					__FUNCTION__,__LINE__,d,cw);
				fclose(obsFile);
				return(-1);
			}
			if (strncmp(CWMAP[i].controlWordString,cwName,MAXCW)==0){
				fprintf(stderr,"%s.%d PROBLEM mapfile in %s cwName conflict %s\n",
					__FUNCTION__,__LINE__,d,cwName);
				fclose(obsFile);
				return(-1);
			}
		}
		strncpy(CWMAP[i].controlWordString,cwName,MAXCW);
		CWMAP[i].cw=cw;
		++CWMAPN;
		cwMapChanged=1;
	}
	fclose(obsFile);
	return(0);
}
	
int processObsDir(char *inDirName){
        struct dirent **file=0;
        int nFiles=0;
	if (handleCwMapUpdate(inDirName)==-1){
	       	return(-1);
	}
        if ((nFiles=scandir(inDirName,&file,isPDD,0))<0){
                fprintf(stderr,"%s.%d scandir %s failed with %s\n",
                        __FUNCTION__,__LINE__,inDirName,strerror(errno));
                return(-1);
        }
        for(int i=0;i<nFiles;++i){
                if (processInFile(inDirName,file[i]->d_name)!=0){
			return(-1);
		}
                free(file[i]);
        }
        return(0);
}
int processTSDir(char *inDirName,time_t oldestModel){
        struct dirent **file=0;
        int nFiles=0;
        if ((nFiles=scandir(inDirName,&file,isTS,0))<0){
                fprintf(stderr,"%s.%d scandir(%s) failed with %s\n",
                        __FUNCTION__,__LINE__,inDirName,strerror(errno));
                return(-1);
        }
        for(int i=0;i<nFiles;++i){
		char FQN[PATH_MAX];
		struct stat dirStat;
		FILE *f=0;
		snprintf(FQN,sizeof(FQN),"%s/%s",inDirName,file[i]->d_name);
		if ((f=fopen(FQN,"re"))==0){
			if (errno != ENOENT){
				fprintf(stderr,"%s.%d fopen %s %s\n",
					__FUNCTION__,__LINE__,
					FQN,strerror(errno));
			}
			return(-1);
		}
		if (fstat(fileno(f),&dirStat)!=0){
				fprintf(stderr,"%s.%d fstat %s\n",
					__FUNCTION__,__LINE__,
					strerror(errno));
				return(-1);
		}
		if (dirStat.st_mtime >= oldestModel){
                	fprintf(stdout,"new %s %d of %d\r",FQN,i+1,nFiles);
                	if (processObsDir(FQN)!=0){
				return(-1);
			}
		} else {
                	fprintf(stdout,"old %s %d of %d\r",FQN,i+1,nFiles);
		}
		fflush(stdout);
                free(file[i]);
        }
	fprintf(stdout,"\n");
        return(0);
}

#define REQARGS 3
int main(int argc, char *argv[]) {
	char obsDir[PATH_MAX],HOST[SHORTSTRING];
	HOST[0]='\0';
	if (argc != REQARGS){
		fprintf(stderr,"%s.%d usage %s --host h \n",
				__FUNCTION__,__LINE__,argv[0]);
	}
	for (size_t i=1;i<argc;++i){
		if (strncmp(argv[i],"--host",strlen("--host"))==0) {
			strncpy(HOST,argv[i+1],PATH_MAX);
			++i;
			continue;
		}
		fprintf(stderr,"%s.%d bad argument %s\n",__FUNCTION__,__LINE__,argv[i]);
		exit(-1);
	}
	if (HOST[0] == '\0') {
		fprintf(stderr,"%s.%d must specify host with --host\n",__FUNCTION__,__LINE__);
		exit(-1);
	}
	char *e=initModels();
	if (e != 0){
		fprintf(stderr,"%s.%d initModels failed %s\n",
				__FUNCTION__,__LINE__,e);
		exit(-1);
	}
	if ((e=initCwMap()) != 0){
		logMessage(stderr,__FUNCTION__,__LINE__,"initCwMap failed %s",e);
		return(-1);
	}
	snprintf(obsDir,sizeof(obsDir),OBSSTORED,HOST);
	if (processTSDir(obsDir,oldestModelTime())!=0){
		return(-1);
	}
	if (cwMapChanged>0){
		reWriteCwMap();
	}
	writeModels();
}
