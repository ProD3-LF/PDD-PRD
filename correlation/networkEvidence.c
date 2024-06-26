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
#include "alert.h"
#include "../common/logMessage.h"
#include "defs.h"
#include "networkEvidence.h"
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
size_t guiltyIPs=0;
typedef struct {
	char name[SHORTSTRING];
	int alertType[MAX_ALERT_TYPES];
	size_t alertTypeX;
} attributionRule;
attributionRule ATTRULE[MAXATTRIBUTIONRULES];
size_t attributionRuleX=0;
void dumpAttributionRules(){
	for (size_t i=0;i<attributionRuleX;++i){
		attributionRule *at=&ATTRULE[i];
		logMessage(stderr,__FUNCTION__,__LINE__,
			"attributionRule[%ld]: %s",i,at->name);
		for (size_t j=0;j<ATTRULE[i].alertTypeX;++j){
			logMessage(stderr,__FUNCTION__,__LINE__,
				" %s",alertType2String(at->alertType[j]));
		}
	}
}
void addAttributionRule(char *r){
	if (attributionRuleX == MAXATTRIBUTIONRULES) {
		logMessage(stderr,__FUNCTION__,__LINE__,
				"Too many attribution rules");
		return;
	}
	attributionRule *at=&ATTRULE[attributionRuleX];
	at->alertTypeX=0;
	size_t i=0;
	for(;r[i]!='\0';++i){
		if (r[i]==' '){break;}
		at->name[i]=r[i];
	}
	if (r[i] == '\0'){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"empty attribution rule %s",r);
		return;
	}
	at->name[i]='\0';
	++i;
	size_t s=i;
	while(1==1){
		if ((r[i]==' ') || (r[i]=='\0')){
			int t=alertType2Int(&r[s]);
			if (t==-1){
				logMessage(stderr,__FUNCTION__,__LINE__,
					"bad alert type in %s",r);
				return;
			}
			at->alertType[at->alertTypeX++]=t;
			s=i+1;
		}
		if (r[i] == '\0'){break;}
		++i;
	}
	attributionRuleX++;
}
int compareAdr(uint32_t IP1,uint32_t IP2){
	if (IP1 < IP2){return(-1);}
	if (IP1 > IP2){return(1);}
	return(0);
}
#define MINRATE .01
void  evaluateOneNe(const int alertType[],int alertTypeX,networkEvidence *ne,time_t now){
	size_t i=0;
	for (;i<alertTypeX;++i){
		getScore(&ne->avg[alertType[i]],now);
		if (ne->avg[alertType[i]].value<MINRATE){
			       	return;
		}
	}
	ne->lastGuilty=now;
	if (ne->guiltyCount == 0){
		++guiltyIPs;
	       	ne->guiltyCount=1;
	}
}
FILE *guiltyFile=0;
void outputGuilty(networkEvidence **ne,size_t *nex,time_t now){
	if (guiltyFile == 0) {
		if ((guiltyFile=fopen("/tmp/GUILTY","w+e"))==0){
			logMessage(stderr,__FUNCTION__,__LINE__,"fopen %s %s","/tmp/GUILTY",strerror(errno));
			return;
		}
	}
	for (size_t i=0;i<*nex;++i){
		if (ne[i]->guiltyCount!=0){
			if (ne[i]->guiltyCount == 1){
				struct in_addr t;
				t.s_addr=ntohl(ne[i]->IP);
				fprintf(guiltyFile,"%lu,Guilty,%s\n",now,inet_ntoa(t));
				fflush(guiltyFile);
				ne[i]->guiltyCount=2;
			}
			if ((now-ne[i]->lastGuilty) > CLEARGUILTTIME){
				struct in_addr t;
				t.s_addr=ntohl(ne[i]->IP);
				ne[i]->lastGuilty=0;
				ne[i]->guiltyCount=0;
				fprintf(guiltyFile,"%lu,UnGuilty,%s\n",now,inet_ntoa(t));
				fflush(guiltyFile);
				guiltyIPs--;
			} else{ne[i]->guiltyCount=2;}
		}
		if ((now-ne[i]->lastAnomaly) > CLEAREVIDENCETIME){
			free(ne[i]);
			for(size_t j=i;j<*nex-1;++j){
				ne[j]=ne[j+1];
			}
			*nex = *nex-1;
			--i;
		}
	}
}
char *evaluateNe(const int alertType[],int alertTypeX,
	networkEvidence **ne, size_t nex,time_t now){
	for(size_t j=0;j<nex;++j){
		size_t i=0;
		for (;i<alertTypeX;++i){
			getScore(&ne[j]->avg[alertType[i]],now);
			if (ne[j]->avg[alertType[i]].value<MINRATE){
			       	break;
			}
		}
		if (i == alertTypeX){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"guilty %08x",ne[j]->IP);
		}
	}
	return(0);
}
void attributeOneClient(char *attackName, networkEvidence *ne,time_t now){
	for (size_t i=0;i<attributionRuleX;++i){
		attributionRule *ar=&ATTRULE[i];
		if (strcmp(ar->name,attackName)!=0){continue;}
		evaluateOneNe(ar->alertType,ar->alertTypeX,ne,now);
	}
}
char *attributeClients(char *attackName, networkEvidence **ne,size_t nex,time_t now){
	for (size_t i=0;i<attributionRuleX;++i){
		attributionRule *ar=&ATTRULE[i];
		if (strcmp(ar->name,attackName)!=0){continue;}
		return(evaluateNe(ar->alertType,ar->alertTypeX,ne,nex,now));
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"no attribution rule for %s",
		attackName);
	return(0);
}
networkEvidence *insertNe(networkEvidence ***ne,int rc,int m, uint32_t IP,size_t *nex,size_t *nen,int alertType,size_t count){
	networkEvidence *e=0;
	networkEvidence **t=0;
	if (*nex == *nen){
		if ((t=(networkEvidence **)realloc(*ne,(*nen+NEINCR)*sizeof(networkEvidence *)))==0){
			logMessage(stderr,__FUNCTION__,__LINE__,"realloc failed");
			return(0);
		}
		*ne=t;
		*nen = *nen+NEINCR;
	} else {
		t = *ne;
	}
	if ((e=malloc(sizeof(networkEvidence)))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"malloc failed");
		return(0);
	}
	e->IP=IP;
	e->count=1;
	for (size_t a=0;a<MAX_ALERT_TYPES;++a){
		e->avg[a].value = 0.0;
		e->avg[a].lastUpdate = 0.0;
	}
	//updateScore(&(e->avg[alertType]),alertTime,count);
	updateScore(&(e->avg[alertType]),time(0),count);
	e->lastAnomaly=time(0);
	e->lastGuilty=0;
	e->guiltyCount=0;
	if (rc < 0){
		for(size_t i=*nex;i>m;--i){
			t[i]=t[i-1];
		}
		t[m]=e;
		*nex = *nex + 1;
		return(e);
	}
	for(int i=*nex;i>m+1;--i){
		t[i]=t[i-1];
	}
	t[m+1]=e;
	*nex = *nex + 1;
	return(e);
}
networkEvidence *addNe(uint32_t IP,int alertType,size_t count,networkEvidence ***ne,size_t *nex,size_t *nen){
	int rc=-1;
	networkEvidence **t=*ne;
	int e = (int)(*nex - 1);
	int s=0;
	int m=0;
	while (s <= e){
		m = (s+e)/2;
		if ((rc=compareAdr(t[m]->IP,IP))==0){
			//updateScore(&((t[m])->avg[alertType]),alertTime,count);
			updateScore(&((t[m])->avg[alertType]),time(0),count);
			(t[m])->count += count;
			(t[m])->lastAnomaly=time(0);
			return(t[m]);
		}
		if (rc < 0) {
				e = m-1;
		} else {
				s = m+1;
		}
	}
	return(insertNe(ne,rc,m,IP,nex,nen,alertType,count));
}
