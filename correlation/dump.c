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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "alert.pb-c.h"
#include "oad_support.h"
#include "defs.h"
void dumpTimeStamp(Cydef__Timestamp *a,char *as){
	if (a->has_usecs) {
		sprintf(as,"timestamp=%d.%d,",
			a->secs,
			a->usecs);
		return;
	}
	sprintf(as,"timestamp=%d,",a->secs);
}
void dumpComponentID(Cydef__ComponentId *a, char *as){
	sprintf(&as[strlen(as)],"component_id->name=%s,",
			a->name);
}

void dumpHostId(Cydef__HostId *a,char *as){
	sprintf(as,"HOSTID:");
	if (a->name) sprintf(&as[strlen(as)],"name=%s",a->name);
	if (a->address) sprintf(&as[strlen(as)],",address=%s",a->address);
	if (a->os) sprintf(&as[strlen(as)],",os=%s",a->os);
	strcat(as,"\n");
}
void dumpOAD(Cydef__OAD *a,char *as){
	sprintf(as,"OAD:");
	dumpTimeStamp(a->timestamp,&as[strlen(as)]);
	dumpComponentID(a->component_id,&as[strlen(as)]);
	if (a->host) dumpHostId(a->host,&as[strlen(as)]);
//	strcat(as,"\n");
}
void dumpContrackConnection(Cydef__ConntrackConnection *a, char *as){
	sprintf(as,"CONNTRACKCONNECTION:\n");
	if (a->protoname) sprintf(&as[strlen(as)],"protoname=%s,",a->protoname);
	sprintf(&as[strlen(as)],"protonum=%d,state=%d, srcip=%s, dstip=%s,sport=%d, dport=%d,f_bytes=%lu,r_bytes=%lu,delta=%u,",
	a->protonum,a->state,a->srcip,a->dstip,a->sport,a->dport,a->f_bytes,a->r_bytes,a->delta);
	dumpTimeStamp(a->start,&as[strlen(as)]);
	strcat(as,"\n");
}
void dumpConntrackObs(Cydef__ConntrackObservable *a, char *as){
	int i;
	sprintf(as,"CONNTRACKOBS:\n");
	for(i=0;i<a->n_connection;++i){
		dumpContrackConnection(a->connection[i],&as[strlen(as)]);
	}
//	strcat(as,"\n");
}
void dumpNetworkObs(Cydef__NetworkObservable *a, char *as){
	sprintf(&as[strlen(as)],"%d,%s,%d,%s,%d",
                        a->packet->protocol,
                        a->packet->src,
                        a->packet->src_port,
                        a->packet->dst,
                        a->packet->dst_port);
	strcat(as,";");
}
void dumpHostObservable(Cydef__HostObservable *a,char *as){
	sprintf(&as[strlen(as)],"HOSTOBS:");
	sprintf(&as[strlen(as)],"sourcePid=%d,",a->process_id);
	sprintf(&as[strlen(as)],"sourceThreadID=%d,",a->thread_id);
	if (a->app_name) sprintf(&as[strlen(as)],"app_name=%s",a->app_name);
	strcat(as,"\n");
}
void getViolations(Cydef__Alert *a,char *v){
	int i;
	strcpy(v,"UNKNOWN");
	if (!a->data) return;
	for(i=0;i<a->n_data;++i){
		if (strcmp(a->data[i]->name,"violations")==0){
			strcpy(v,a->data[i]->value->string_data);
		}
	}
}
void getClass(Cydef__Alert *a,int *nrm,int *hrm,int *pm){
	int i;
	*nrm=*hrm=*pm=0;
	if (!a->data) return;
	for(i=0;i<a->n_data;++i){
		if (strcmp(a->data[i]->name,"NRM")==0){
			*nrm=a->data[i]->value->int_data;
			continue;
		}
		if (strcmp(a->data[i]->name,"HRM")==0){
			*hrm=a->data[i]->value->int_data;
			continue;
		}
		if (strcmp(a->data[i]->name,"PM")==0){
			*pm=a->data[i]->value->int_data;
			continue;
		}
	}
}
void dumpNvPairs(Cydef__Alert *a,char *as){
	int i;
	if (!a->data) return;
	sprintf(as,"NVPAIRS:");
	for(i=0;i<a->n_data;++i){
		strcat(as,a->data[i]->name);
		strcat(as," ");
		if ((strcmp(a->data[i]->name,"NRM")==0) ||
		     (strcmp(a->data[i]->name,"HRM")==0) ||
		     (strcmp(a->data[i]->name,"PM")==0)){
			char b[32];
			sprintf(b,"%d",a->data[i]->value->int_data);
			strcat(as,b);
		}
		if (strcmp(a->data[i]->name,"violations")==0){
			strcat(as,a->data[i]->value->string_data);
		}
		strcat(as," ");
	}
}
void dumpEvidence(Cydef__Alert *a,char *as){
	int i;
	if (!a->evidence) return;
	sprintf(as,"EVIDENCE:");
	for(i=0;i<a->n_evidence;++i){
		sprintf(&as[strlen(as)],"\nevidence[%d]: ",i);
		if (a->evidence[i]->oad) dumpOAD(a->evidence[i]->oad,&as[strlen(as)]);
		if (a->evidence[i]->network) dumpNetworkObs(a->evidence[i]->network,&as[strlen(as)]);
		if (a->evidence[i]->host) dumpHostObservable(a->evidence[i]->host,&as[strlen(as)]);
		if (a->evidence[i]->conntrack) dumpConntrackObs(a->evidence[i]->conntrack,&as[strlen(as)]);
		if (i>100){
			strcat(as,"AND SO ON ...");
			break;
		}
	}
	strcat(as,"\n");
}

void dumpImpact(Cydef__Alert *a,char *as){
	sprintf(as,"Impact=%d,",a->impact);
}
void dumpViolation(Cydef__Alert *a,char *as){
	sprintf(as,"violation=%s,",a->violation);
}
void dumpConfidence(Cydef__Alert *a,char *as){
	sprintf(as,"Confidence=%d,",a->confidence);
}

void logAlertOld(Cydef__Alert* a,FILE *f){
	char *as;
	if ((as = malloc(100000))==0){
		fprintf(stderr,"%s.%d malloc failed\n",__FUNCTION__,__LINE__);
		return;
	}
	as[0]='\0';
	sprintf(as,"--------------ALERT at %ld----------------\n",time(0));
	if (a->oad) dumpOAD(a->oad,&as[strlen(as)]);
	if (a->source) dumpHostObservable(a->source,&as[strlen(as)]);
	if (a->target) dumpHostObservable(a->target,&as[strlen(as)]);
	dumpViolation(a,&as[strlen(as)]);
	dumpEvidence(a,&as[strlen(as)]);
	if (a->has_impact) dumpImpact(a,&as[strlen(as)]);
	if (a->has_confidence) dumpConfidence(a,&as[strlen(as)]);
	sprintf(&as[strlen(as)],"\n--------------ALERTEND--------------\n");
	fprintf(f,"%s\n",as);
	fflush(f);
	free(as);
}
void logAlert(Cydef__Alert* a,FILE *f){
	char firstPart[512],lastPart[10000],source[512],target[512],container[512],violations[10000];
	int i,ne=0,nrm,hrm,pm;
	if ((a->source) && (a->source->app_name)) {
		snprintf(source,512,"%s.%s.%d.%d",
			a->source->container_name,a->source->app_name,a->source->process_id,a->source->container_process_id);
	} else {
		snprintf(source,512,"UNKOWNSOURCE.0.0");
	}
	if ((a->target) && (a->target->app_name)) {
		snprintf(target,512,"%s.%s.%d.%d",
			a->target->container_name,a->target->app_name,a->target->process_id,a->target->container_process_id);
	} else {
		snprintf(target,512,"UNKOWNTARGET.0.0");
	}
	if ((a->oad) && (a->oad->respond_to)
			 && (a->oad->respond_to->container_name)){
		snprintf(container,512,"%s", a->oad->respond_to->container_name);
	} else {
		snprintf(container,512,"UNKNOWN");
	}
	getClass(a,&nrm,&hrm,&pm);
//	getViolations(a,violations);
	snprintf(firstPart,512,"%d",a->oad->timestamp->secs);
//	snprintf(lastPart,10000,"%s %d NRM %d HRM %d PM %d violations %s",a->violation,a->confidence,nrm,hrm,pm,violations);
	snprintf(lastPart,10000,"%s %d NRM %d HRM %d PM %d",a->violation,a->confidence,nrm,hrm,pm);
	for(i=0;i<a->n_evidence;++i){
		if (a->evidence[i]->network){
			++ne;
			fprintf(f,"NE%d %s %s %s %s %d %s %d %s %d %s\n",
				i,
				firstPart,
				source,
				target,
				container,
				a->evidence[i]->network->packet->protocol,
				a->evidence[i]->network->packet->src,
				a->evidence[i]->network->packet->src_port,
				a->evidence[i]->network->packet->dst,
				a->evidence[i]->network->packet->dst_port,
				lastPart);
		}
		if (a->evidence[i]->host){
			++ne;
			fprintf(f,"HE%d %s %s %s %s %d %d %s %s %d %s\n",
				i,
				firstPart,
				source,target,container,
				a->evidence[i]->host->process_id,
				a->evidence[i]->host->container_process_id,
				a->evidence[i]->host->user,
				a->evidence[i]->host->app_name,
				a->evidence[i]->host->thread_id,
				lastPart);
		}
	}
	if (ne == 0) {
		fprintf(f,"E0 %s %s %s %s %d %s %d %s %d %s\n",
			firstPart,source,target,container,0,"0",0,"0",0,
				lastPart);
	}
}
void dumpAlert(Cydef__Alert* a){
	char *as;
	if ((as = malloc(500000))==0){
		fprintf(stderr,"%s.%d malloc failed\n",__FUNCTION__,__LINE__);
		return;
	}
	as[0]='\0';
	sprintf(as,"--------------ALERT----------------\n");
	if (a->oad) dumpOAD(a->oad,&as[strlen(as)]);
	if (a->source) dumpHostObservable(a->source,&as[strlen(as)]);
	if (a->target) dumpHostObservable(a->target,&as[strlen(as)]);
	dumpViolation(a,&as[strlen(as)]);
	dumpEvidence(a,&as[strlen(as)]);
	if (a->has_impact) dumpImpact(a,&as[strlen(as)]);
	if (a->has_confidence) dumpConfidence(a,&as[strlen(as)]);
	sprintf(&as[strlen(as)],"\n--------------ALERTEND--------------\n");
	printf("%s.%d %s\n",__FUNCTION__,__LINE__,as);
	fflush(stdout);
	free(as);
}
