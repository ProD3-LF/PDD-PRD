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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "defs.h"
#include "../common/pddObs.h"
#include "../common/logMessage.h"
#define FIN 0x1
#define SYN 0x2
#define RST 0x4
#define PSH 0x8
#define ACK 0x10
#define URG 0x20
uint8_t tcpFlagsToInt(char *f){
	uint8_t t=0;
	if (strstr(f,"U")!=0) t |= URG;
	if (strstr(f,".")!=0) t |= ACK;
	if (strstr(f,"P")!=0) t |= PSH;
	if (strstr(f,"R")!=0) t |= RST;
	if (strstr(f,"S")!=0) t |= SYN;
	if (strstr(f,"F")!=0) t |= FIN;
	return(t);
}


int parseTcpLine(char *b,double *obsTime,in_addr_t *serverPDDIP,
		in_addr_t *clientPDDIP,unsigned int *serverPDDPort,
		unsigned int *clientPDDPort, in_addr_t *serverPRDIP,
		unsigned int *serverPRDPort,
	       	uint8_t *flag,int8_t *direction){
	size_t t,i;
	char ip1S[512],ip2S[512],flagS[32];
	unsigned int port1,port2;
	struct in_addr ip1,ip2;
	if (sscanf(b,"%lg IP %s > %s Flags [%s]",obsTime,ip1S,ip2S,flagS)!=4){
		logMessage(stderr,__FUNCTION__,__LINE__,"Cannot parse: %s",b);
		exit(-1);
	}
	t=strlen(flagS)-1;
	if ((flagS[t]!=',') || (flagS[t-1]!=']')){
		logMessage(stderr,__FUNCTION__,__LINE__,"bad flag %s in %s",flagS,b);
		exit(-1);
	}
	flagS[t-1]='\0';
	*flag=tcpFlagsToInt(flagS);
	t=strlen(ip1S)-1;
	for(i=t;i>0;--i){
		if (ip1S[i]=='.'){
			++i;
		       	break;
		}
	}
	if (i==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"no port1 in %s",b);
		exit(-1);
	}
	port1=ntohs(atoi(&ip1S[i]));
	ip1S[i-1]='\0';
	t=strlen(ip2S)-1;
	for(i=t;i>0;--i){
		if (ip2S[i]=='.'){
			++i;
		       	break;
		}
	}
	if (i==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"no port2 in %s",b);
		exit(-1);
	}
	port2=ntohs(atoi(&ip2S[i]));
	ip2S[i-1]='\0';
	if (inet_aton(ip1S,&ip1)==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"bad ip1 in %s",b);
		exit(-1);
	}
	if (inet_aton(ip2S,&ip2)==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"bad ip2 in %s",b);
		exit(-1);
	}
	if (knownPddServer(port1,ip1.s_addr)){
		*direction=Sent;
		*serverPDDIP=ip1.s_addr;
		*serverPDDPort=port1;
		*clientPDDIP=ip2.s_addr;
		*clientPDDPort=ntohs(port2);
	}
	else if (knownPddServer(port2,ip2.s_addr)){
		*direction=Received;
		*serverPDDIP=ip2.s_addr;
		*serverPDDPort=port2;
		*clientPDDIP=ip1.s_addr;
		*clientPDDPort=ntohs(port1);
	}
	else{
	       	logMessage(stderr,__FUNCTION__,__LINE__,
			"%s.%d no known PDD server in %s", b);
		return(-1);
	}
	if (knownPrdServer(port1,ip1.s_addr)){
		*direction=Sent;
		*serverPRDIP=ip1.s_addr;
		*serverPRDPort=port1;
	}
	else if (knownPddServer(port2,ip2.s_addr)){
		*direction=Received;
		*serverPRDIP=ip2.s_addr;
		*serverPRDPort=port2;
	}
	else{
	       	logMessage(stderr,__FUNCTION__,__LINE__,
			"%s.%d no known PRD server in %s",
			b);
		return(-1);
	}
	return(0);
}



int parseTcpLinePrd(char *b,double *obsTime,in_addr_t *serverIP,unsigned int *serverPort,uint8_t *flag,int8_t *direction){
	size_t t,i;
	char ip1S[512],ip2S[512],flagS[32];
	unsigned int port1,port2;
	struct in_addr ip1,ip2;
	if (sscanf(b,"%lg IP %s > %s Flags [%s]",obsTime,ip1S,ip2S,flagS)!=4){
		logMessage(stderr,__FUNCTION__,__LINE__,"Cannot parse: %s",b);
		exit(-1);
	}
	t=strlen(flagS)-1;
	if ((flagS[t]!=',') || (flagS[t-1]!=']')){
		logMessage(stderr,__FUNCTION__,__LINE__,"bad flag %s in %s",flagS,b);
		exit(-1);
	}
	flagS[t-1]='\0';
	*flag=tcpFlagsToInt(flagS);
	t=strlen(ip1S)-1;
	for(i=t;i>0;--i){
		if (ip1S[i]=='.'){
			++i;
		       	break;
		}
	}
	if (i==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"no port1 in %s",b);
		exit(-1);
	}
	port1=ntohs(atoi(&ip1S[i]));
	ip1S[i-1]='\0';
	t=strlen(ip2S)-1;
	for(i=t;i>0;--i){
		if (ip2S[i]=='.'){
			++i;
		       	break;
		}
	}
	if (i==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"no port2 in %s",b);
		exit(-1);
	}
	port2=ntohs(atoi(&ip2S[i]));
	ip2S[i-1]='\0';
	if (inet_aton(ip1S,&ip1)==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"bad ip1 in %s",b);
		exit(-1);
	}
	if (inet_aton(ip2S,&ip2)==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"bad ip2 in %s",b);
		exit(-1);
	}
	if (knownPrdServer(port1,ip1.s_addr)){
		*direction=Sent;
		*serverIP=ip1.s_addr;
		*serverPort=port1;
		return(0); 
	}
	if (knownPrdServer(port2,ip2.s_addr)){
		*direction=Received;
		*serverIP=ip2.s_addr;
		*serverPort=port2;
		return(0); 
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"%s.%d no known server in %s",
			b);
	return(-1);
}
int parseTcpLinePdd(char *b,double *obsTime,in_addr_t *serverIP,in_addr_t *clientIP,unsigned int *serverPort,unsigned int *clientPort, uint8_t *flag,int8_t *direction){
	size_t t,i;
	char ip1S[512],ip2S[512],flagS[32];
	unsigned int port1,port2;
	struct in_addr ip1,ip2;
	if (sscanf(b,"%lg IP %s > %s Flags [%s]",obsTime,ip1S,ip2S,flagS)!=4){
		logMessage(stderr,__FUNCTION__,__LINE__,"Cannot parse: %s",b);
		exit(-1);
	}
	t=strlen(flagS)-1;
	if ((flagS[t]!=',') || (flagS[t-1]!=']')){
		logMessage(stderr,__FUNCTION__,__LINE__,"bad flag %s in %s",flagS,b);
		exit(-1);
	}
	flagS[t-1]='\0';
	*flag=tcpFlagsToInt(flagS);
	t=strlen(ip1S)-1;
	for(i=t;i>0;--i){
		if (ip1S[i]=='.'){
			++i;
		       	break;
		}
	}
	if (i==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"no port1 in %s",b);
		exit(-1);
	}
	port1=ntohs(atoi(&ip1S[i]));
	ip1S[i-1]='\0';
	t=strlen(ip2S)-1;
	for(i=t;i>0;--i){
		if (ip2S[i]=='.'){
			++i;
		       	break;
		}
	}
	if (i==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"no port2 in %s",b);
		exit(-1);
	}
	port2=ntohs(atoi(&ip2S[i]));
	ip2S[i-1]='\0';
	if (inet_aton(ip1S,&ip1)==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"bad ip1 in %s",b);
		exit(-1);
	}
	if (inet_aton(ip2S,&ip2)==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"bad ip2 in %s",b);
		exit(-1);
	}
	if (knownPddServer(port1,ip1.s_addr)){
		*direction=Sent;
		*serverIP=ip1.s_addr;
		*serverPort=port1;
		*clientIP=ip2.s_addr;
		*clientPort=ntohs(port2);
		return(0); 
	}
	if (knownPddServer(port2,ip2.s_addr)){
		*direction=Received;
		*serverIP=ip2.s_addr;
		*serverPort=port2;
		*clientIP=ip1.s_addr;
		*clientPort=ntohs(port1);
		return(0); 
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"%s.%d no known server in %s",
			b);
	return(-1);
}
