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
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "defs.h"
#include "../common/decay.h"
#include "baseProg.h"
#include "alert.pb-c.h"
#include "oad_support.h"
extern int AGETHRESHOLD;
void pruneOldEvidence(baseProg *b){
	time_t now = time(0);
	fprintf(stderr,"%s.%d %d reduced to ",__FUNCTION__,__LINE__,b->nEvidence);
	for(int i=0;i<b->nEvidence;++i){
		Cydef__Observable *e = b->evidence[i];
		if ((e->oad!=0)&&(e->oad->timestamp!=0)){
			if ((e->oad->timestamp->secs + AGETHRESHOLD) < now){
                		free(e->oad);
                		e->oad=0;
                		if (e->network !=0) free(e->network);
                		e->network=0;
                		if (e->host !=0) free(e->host);
                		e->host=0;
                		if (e->conntrack !=0) free(e->conntrack);
                		e->conntrack=0;
				free(e);
				--b->nEvidence;
				for(int j=i;j<b->nEvidence;++j){
					b->evidence[j]=b->evidence[j+1];
				}
				--i;
			}
		}
	}
	fprintf(stderr,"%d\n",b->nEvidence);
}
int newHostEvidence(Cydef__HostObservable *he, Cydef__Observable *o[],int n,int count[]){
	int i;
	for (i=0;i<n;++i){
		if (o[i]->host){
			Cydef__HostObservable *t2=o[i]->host;
			if ((he->process_id == t2->process_id) &&
				(he->thread_id == t2->thread_id)){
				if (he->app_name==0){
					if (he->app_name==0){
						++count[i];
						 return(1);
					}
				} else {
					if ((t2->app_name !=0) &&
						(strncmp(t2->app_name,
							he->app_name,32)==0)){
								++count[i];
								return(1);
					}
				}
			}
		}
	}
	return(0);
}
int compareConntrackConnections(Cydef__ConntrackConnection *c1,Cydef__ConntrackConnection *c2){
	if (c1->protonum!=c2->protonum) return(-1);
	if (c1->sport != c2->sport) return(-4);
	if (c1->dport != c2->dport) return(-4);
	if (c1->state != c2->state) return(-2);
	if (strcmp(c1->srcip,c2->srcip)!=0) return(-3);
	return(0);
}
int newEvidence(Cydef__Observable *ne, Cydef__Observable *o[],int n,int *e,int count[]){
	int i;
	if (ne == 0) {
		fprintf(stderr,"%s.%d ne is 0\n",__FUNCTION__,__LINE__);
		return(1);
	}
	if (ne->conntrack){
		int miss=0;
		for (i=0;i<n;++i){
			int k;
			if (!o[i]->conntrack) continue;
			Cydef__ConntrackObservable *t1=ne->conntrack;
			Cydef__ConntrackObservable *t2=o[i]->conntrack;
			for(k=0;k<t1->n_connection;++k){
				int j;
				Cydef__ConntrackConnection *c1=
							t1->connection[k];
				for(j=0;j<t2->n_connection;++j){
					Cydef__ConntrackConnection *c2=
							t2->connection[j];
					if (compareConntrackConnections(c1,c2)==0){
						if ((c2->f_bytes!=c1->f_bytes) ||
							(c2->r_bytes!=c1->r_bytes)) {
							c2->f_bytes=c1->f_bytes;
							c2->r_bytes=c1->r_bytes;
							c2->delta=c1->delta;
							++count[i];
							fprintf(stderr,"%s.%d changed\n",__FUNCTION__,__LINE__);
						} else {
							fprintf(stderr,"%s.%d unchanged\n",__FUNCTION__,__LINE__);
						}
						break;
					}
				}
				if (j==t2->n_connection){
					o[i]->conntrack->connection=
						realloc(
					  	   o[i]->conntrack->connection,
						     sizeof(c1)*
						       (t2->n_connection+1));
					t2=o[i]->conntrack;
					t2->connection[t2->n_connection++]=c1;
					*e++;
					++miss;
					count[i]=1;
				}
			}
		}
		if (miss==0) return(1);
		return(0);
	}
	if (ne->network){
/*		if (ne->network->packet->src_port==-1){
			fprintf(stderr,"%s.%d src_port is -1\n",
				__FUNCTION__,__LINE__);
//			 return(1);
		}
//		if (ne->network->packet->dst_port==-1){
//			fprintf(stderr,"%s.%d dst_port is -1\n",
//				__FUNCTION__,__LINE__);
//			 return(1);
		}*/
		for (i=0;i<n;++i){
			if (!o[i]->network) continue;
			Cydef__NetworkObservable *t1=ne->network;
			Cydef__NetworkObservable *t2=o[i]->network;
			if ((t1->packet->src_port==t2->packet->src_port)&&
			    (t1->packet->dst_port==t2->packet->dst_port)&&
			    (strcmp(t1->packet->src,t2->packet->src)==0) &&
			    (strcmp(t1->packet->dst,t2->packet->dst)==0)){
				if ((o[i]->oad!=0)&&(o[i]->oad->timestamp!=0)){
					if ((ne->oad!=0)&&(ne->oad->timestamp!=0)){
						if ((ne->oad->timestamp->secs-o[i]->oad->timestamp->secs)>AGETHRESHOLD) {
							count[i]=0;//if all the existing evidence is too old do not count it
						}
						o[i]->oad->timestamp->secs =
							ne->oad->timestamp->secs;
					} else {
						fprintf(stderr,"%s.%d ne has no timestamp\n",__FUNCTION__,__LINE__);
						if ((time(0)-o[i]->oad->timestamp->secs)>AGETHRESHOLD) {
							count[i]=0;//if all the existing evidence is too old do not count it
						}
					}
				} else {
					fprintf(stderr,"%s.%d oe has no timestamp\n",
						__FUNCTION__,__LINE__);
				}
				++count[i];
				return(1);
			}
		}
		return(0);
	}
	if (ne->host){
		for (i=0;i<n;++i){
			if (!o[i]->host) continue;
			Cydef__HostObservable *t1=ne->host;
			Cydef__HostObservable *t2=o[i]->host;
			if ((t1->process_id == t2->process_id) &&
				(t1->thread_id == t2->thread_id)){
				if (t1->app_name==0){
					if (t2->app_name==0){
						if ((o[i]->oad!=0)&&(o[i]->oad->timestamp!=0)){
							if ((ne->oad->timestamp->secs-o[i]->
									oad->timestamp->secs)>AGETHRESHOLD) {
								count[i]=0;
							}
							o[i]->oad->timestamp->secs =
								ne->oad->timestamp->secs;
						} else{
							fprintf(stderr,"%s.%d no timestamp\n",
								__FUNCTION__,__LINE__);
						}
						++count[i];
						return(1);
					}
				} else {
					if ((t2->app_name !=0) &&
						(strncmp(t2->app_name,
							t1->app_name,32)==0)){
								if ((o[i]->oad!=0)&&(o[i]->oad->timestamp!=0)){
									if ((ne->oad->timestamp->secs-o[i]->
											oad->timestamp->secs)
												>AGETHRESHOLD) {
										count[i]=0;
									}
									o[i]->oad->timestamp->secs = ne->oad->timestamp->secs;
								} else {
									fprintf(stderr,"%s.%d no timestamp\n",
										__FUNCTION__,__LINE__);
								}
								++count[i];
								return(1);
					}
				}
			}
		}
		return(0);
	}
	fprintf(stderr,"%s.%d unknown evidence type\n",__FUNCTION__,__LINE__);
	return(1);
}
Cydef__Observable *hostToObservable(Cydef__HostObservable *h){
	Cydef__Observable *obs = cydef__observable__create();
	obs->host = h;
	return(obs);
}
int addEvidence(baseProg *b, Cydef__Alert *a){
	int i,ne=0;
	if (a->source){
		if (newHostEvidence(a->source,b->evidence,b->nEvidence,b->evidenceCount)==0) {
			if (b->nEvidence == b->maxEvidence){
				if ((b->evidence=
					realloc(b->evidence,sizeof(a->evidence)*
						(b->nEvidence+100)))==0){
					fprintf(stderr,"%s.%d malloc failure\n",
						__FUNCTION__,__LINE__);
					return(0);
				}
				if ((b->evidenceCount=
					realloc(b->evidenceCount,sizeof(int)*
						(b->nEvidence+100)))==0){
					fprintf(stderr,"%s.%d malloc failure\n",
						__FUNCTION__,__LINE__);
					return(0);
				}
				b->maxEvidence+=100;
			}
			++ne;
			b->evidenceCount[b->nEvidence]=1;
			b->evidence[b->nEvidence++]=hostToObservable(a->source);
//take the copied source out of the alert so it dont get freed
			a->source=0;
		}
	}
	if (a->target){
		if (newHostEvidence(a->target,b->evidence,b->nEvidence,b->evidenceCount)==0) {
			if (b->nEvidence == b->maxEvidence){
				if ((b->evidence=
					realloc(b->evidence,sizeof(a->evidence)*
						(b->nEvidence+100)))==0){
					fprintf(stderr,"%s.%d malloc failure\n",
						__FUNCTION__,__LINE__);
					return(0);
				}
				if ((b->evidenceCount=
					realloc(b->evidenceCount,sizeof(int)*
						(b->nEvidence+100)))==0){
					fprintf(stderr,"%s.%d malloc failure\n",
						__FUNCTION__,__LINE__);
					return(0);
				}
				b->maxEvidence+=100;
			}
			++ne;
			b->evidenceCount[b->nEvidence]=1;
			b->evidence[b->nEvidence++]=hostToObservable(a->target);
//take the copied target out of the alert so it dont get freed
			a->target=0;
		}
	}
	if (!a->evidence) return(ne);
	for(i=0;i<a->n_evidence;++i){
		if (newEvidence(a->evidence[i],
				b->evidence,b->nEvidence,&ne,b->evidenceCount)==0) {
			if (b->nEvidence == b->maxEvidence){
				if ((b->evidence=
					realloc(b->evidence,sizeof(a->evidence)*
						(b->nEvidence+100)))==0){
					fprintf(stderr,"%s.%d malloc failure\n",
						__FUNCTION__,__LINE__);
					return(0);
				}
				if ((b->evidenceCount=
					realloc(b->evidenceCount,sizeof(int)*
						(b->nEvidence+100)))==0){
					fprintf(stderr,"%s.%d malloc failure\n",
						__FUNCTION__,__LINE__);
					return(0);
				}
				b->maxEvidence+=100;
			}
			++ne;
			b->evidenceCount[b->nEvidence]=1;
			b->evidence[b->nEvidence++]=a->evidence[i];
//take the copied evidence out of the alert so it dont get freed
			a->evidence[i]=0;
		}
	}
//clean up the evidence list in case any evidence was taken out
	for(i=0;i<a->n_evidence;++i){
		if (a->evidence[i] == 0){
			int k;
			for (k=i;k<a->n_evidence-1;++k){
				a->evidence[k]=a->evidence[k+1];
			}
			--a->n_evidence;
			--i;
		}
	}
	return(ne);
}
