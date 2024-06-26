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
#include "../common/config.h"
#include "../common/logMessage.h"
#include "alert.h"
#include "classifier.h"
#include "correlatorFileDefs.h"
#include "defs.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
extern size_t guiltyIPs;
extern int processAlert(void *a);
time_t STARTTIME=0;
char MYHOSTNAME[HOST_NAME_MAX+1];
size_t  MYHOSTNAMELEN=0;
struct in_addr MYIP;
size_t ALERTSIN=0;
size_t ATTACKS=0;
pthread_mutex_t CS;
int max(int a,int b){
	if (a > b){return(a);}
	return(b);
}
void handleSig(int s){
	logMessage(stderr,__FUNCTION__,__LINE__,"got %d",s);
}
void enterCS(){
	pthread_mutex_lock(&CS);
}
void exitCS(){
	pthread_mutex_unlock(&CS);
}
int LOGIRFD=0;
void logIR(char *b, size_t n){
        if (LOGIRFD == 0){
                if ((LOGIRFD=open("/tmp/IR",O_RDWR|O_CREAT|O_APPEND|O_CLOEXEC,S_IRUSR|S_IWUSR))<0){//NOLINT(hicpp-signed-bitwise)
                        printf("ERROR: logIR cannot open /tmp/IR\n");
                        LOGIRFD = 0;
                        return;
                }
        }
        if (write(LOGIRFD,b,n) != (ssize_t) n){
                printf("ERROR: logIR cannot write /tmp/IR\n");
        }
}
FILE *alertsRateFile;
void initFiles(){
	char b[PATH_MAX];
	snprintf(b,PATH_MAX,"/bin/mkdir -p " ALERTSRATEDIR,MYHOSTNAME,STARTTIME);
	if (RECORD==1) {
		if (system(b)==-1){ //NOLINT(cert-env33-c)
			logMessage(stderr,__FUNCTION__,__LINE__,"system  %s %s",
					b,strerror(errno));
			alertsRateFile=stderr;
		}
		snprintf(b,PATH_MAX,ALERTSRATEDIR"/AR.alertsRate",MYHOSTNAME,STARTTIME);
        	if ((alertsRateFile=fopen(b,"ae"))==0){
                	logMessage(stderr,__FUNCTION__,__LINE__,
					"fopen %s failed with %s",
                        	b,strerror(errno));
                	alertsRateFile=stderr;
        	}
	} else{alertsRateFile=stderr;}
}
extern void allClear();
typedef struct cleanupArgs_ {
} cleanupArgs;
extern void pruneBP();
FILE *alertsInFile,*alertsOutFile,*ssvFile,*gFile;
#define CLEANINTERVAL 10
static void *cleanup(){
	while (1==1){
		fprintf(alertsInFile,"%s.%d %ld %s-%d-%s %d\n",
                        __FUNCTION__,__LINE__,time(0),"correlator",getpid(),"HEARTBEAT",0);
		enterCS();
		pruneBP();
		allClear();
		exitCS();
		sleep(CLEANINTERVAL);
	}
	return(0);
}
void processPrd(PRDALERT *a){
	ALERT c;
	strcpy(c.message_name,"prdAlert");
	c.t.prdAlert=a;
	processAlert(&c);
}
void processSld(SLDALERT *a){
	ALERT c;
	strcpy(c.message_name,"sldAlert");
	c.t.sldAlert=a;
	processAlert(&c);
}
void processPdd(PDDALERT *a){
	ALERT c;
	strcpy(c.message_name,"pddAlert");
	c.t.pddAlert=a;
	processAlert(&c);
}
int readFromFifo(int fd,void **d){
	ssize_t toRead=0;
	ssize_t haveRead=0;
	ssize_t n1=0;
	while ((n1=read(fd,&toRead,sizeof(toRead)))==-1){
		logMessage(stderr,__FUNCTION__,__LINE__,
			"read fifo %d failed with %s",
			fd,strerror(errno));
		if (errno == EAGAIN){continue;}
		return(-2);
	}
	if (n1 != sizeof(toRead)){
		logMessage(stderr,__FUNCTION__,__LINE__,
			"read fifo %d %lu of %d",
			fd,n1,sizeof(toRead));
		return(-2);
	}
	if ((*d=malloc(toRead+MALLOCPAD))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"malloc failed(%lu)\n",toRead+MALLOCPAD);
		*d=0;
		return(-2);
	}
	unsigned char *p=(unsigned char *)*d;
	while (toRead > 0){
		ssize_t n=read(fd,&p[haveRead],toRead);
		if (n==-1){
			if (errno == EAGAIN){continue;}
			logMessage(stderr,__FUNCTION__,__LINE__,
				"read fifo %d failed with %s",
				fd,strerror(errno));
			return(-1);
		}
		if (n==0){
			logMessage(stderr,__FUNCTION__,__LINE__,
				"writer closed fifo %d",fd);
			close(fd);
			return(-1);
		}
		toRead -=n;
		haveRead += n;
	}
	p[haveRead]='\0';
	return(0);
}
void setupLogs(){
	char b[LONGSTRING];
	snprintf(b,LONGSTRING,"/bin/mkdir -p %s",LOGFILEDIR);
	int rc=system(b);//NOLINT(cert-env33-c)
	if (rc==-1){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"system %s" ,strerror(errno));
		gFile=alertsInFile=alertsOutFile=stderr;
		return;
	}
/*        if (WIFEXITED(rc)) {
#ifndef PUBLIC
              pd3log_error("system(%s)=%d",b,WEXITSTATUS(rc));
#else
              logMessage(stderr,__FUNCTION__,__LINE__,
	      		"system(%s)=%d",b,WEXITSTATUS(rc));
#endif
        }*/
        if (WIFSIGNALED(rc)) { //NOLINT(hicpp-signed-bitwise)
                logMessage(stderr,__FUNCTION__,__LINE__,
				"system(%s) killed by %d",b,WTERMSIG(rc)); //NOLINT(hicpp-signed-bitwise)
                return;
        }
        if (WIFSTOPPED(rc)) { //NOLINT(hicpp-signed-bitwise)
                logMessage(stderr,__FUNCTION__,__LINE__,
				"system(%s) stopped by %d",b,WSTOPSIG(rc));//NOLINT(hicpp-signed-bitwise)
                return;
        }
        if (WIFCONTINUED(rc)) { //NOLINT(hicpp-signed-bitwise)
                logMessage(stderr,__FUNCTION__,__LINE__,
				"system(%s) continued",b);
                return;
        }
	if ((gFile=fopen("/tmp/CORRELATORG","w+e"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"fopen %s %s","/tmp/CORRELATORG",strerror(errno));
		exit(-1);
	}
	snprintf(b,LONGSTRING,"%s/alertsIn.%ld",LOGFILEDIR,STARTTIME);
	if ((alertsInFile=fopen(b,"w+e"))==0){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"fopen %s %s",b,strerror(errno));
			alertsInFile=stderr;
	}
	snprintf(b,LONGSTRING,"%s/alertsOut.%ld",LOGFILEDIR,STARTTIME);
	if ((alertsOutFile=fopen(b,"w+e"))==0){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"fopen %s %s" ,b,strerror(errno));
			alertsOutFile=stderr;
	}
	setlinebuf(stdout);
	setlinebuf(stderr);
	setlinebuf(alertsInFile);
	setlinebuf(alertsOutFile);
	fprintf(alertsInFile,"THIS IS ALERTS IN\n");
	fprintf(alertsOutFile,"THIS IS ALERTS OUT\n");
	snprintf(b,LONGSTRING,"/tmp/correlator.%ld.ssv",STARTTIME);
	if ((ssvFile=fopen(b,"w+e"))==0){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"fopen %s %s",b,strerror(errno));
	}
}
char currentAttackName[MAXATTACKS][LONGSTRING];
size_t currentAttackNameX=0;
void correlatorMain() {
	cleanupArgs cua;
	pthread_t t=0;
	STARTTIME=time(0);
	for(int i=0;i<=SIGSYS;++i){
		if ((i!=SIGINT) && (i != SIGSEGV)){signal(i,handleSig);}
	}
	setupLogs();
	fprintf(stdout,
		"\e[4m\e[1m             CORRELATOR             \e[m\n");
	fprintf(stdout, "%10s %10s %10s\n","TIME","ATTACKS","ATTACKNAME");
        gethostname(MYHOSTNAME,HOST_NAME_MAX);
        MYHOSTNAME[HOST_NAME_MAX]=0;
        MYHOSTNAMELEN=strnlen(MYHOSTNAME,LONGSTRING);
	FILE *configFile=fopen(CONFIGFILE,"r+e");
	if (configFile==0){
		logMessage(stderr,__FUNCTION__,__LINE__,"fopen(%s): %s\n",
				CONFIGFILE,strerror(errno));
		exit(-1);
	}
	configureCorrelator(configFile);
        setMyIp(&MYIP);
        initFiles();
        initGenericFormulas();
        initGenericScenarios();
        initClassifier();
        initAlertRate();
        initBP();
	pthread_create(&t,0,cleanup,&cua);
#define FIFOWAITPERIOD 1000
	while (access("/tmp/CORRELATORPRDFifo",R_OK)==-1){
	    usleep(FIFOWAITPERIOD);
	}
	while (access("/tmp/CORRELATORPDDFifo",R_OK)==-1){
	    usleep(FIFOWAITPERIOD);
	}
	while (access("/tmp/CORRELATORSLDFifo",R_OK)==-1){
	    usleep(FIFOWAITPERIOD);
	}
	int prdFd=0;
	int pddFd=0;
	int sldFd=0;
	while ((prdFd = open("/tmp/CORRELATORPRDFifo", O_RDONLY|O_NONBLOCK|O_CLOEXEC))==-1){ //NOLINT(hicpp-signed-bitwise)
		logMessage(stderr,__FUNCTION__,__LINE__,
				"FIFO open failed %s",strerror(errno));
		while (access("/tmp/CORRELATORPRDFifo",R_OK)==-1){
	    		logMessage(stderr,__FUNCTION__,__LINE__,
					"waiting for fifo");
	    		usleep(FIFOWAITPERIOD);
		}
	}
	while ((pddFd = open("/tmp/CORRELATORPDDFifo", O_RDONLY|O_NONBLOCK|O_CLOEXEC))==-1){ //NOLINT(hicpp-signed-bitwise)
		logMessage(stderr,__FUNCTION__,__LINE__,
				"FIFO open failed %s",strerror(errno));
		while (access("/tmp/CORRELATORPDDFifo",R_OK)==-1){
	    		logMessage(stderr,__FUNCTION__,__LINE__,
					"waiting for fifo");
	    		usleep(FIFOWAITPERIOD);
		}
	}
	while ((sldFd = open("/tmp/CORRELATORSLDFifo", O_RDONLY|O_NONBLOCK|O_CLOEXEC))==-1){ //NOLINT(hicpp-signed-bitwise)
		logMessage(stderr,__FUNCTION__,__LINE__,
				"FIFO open failed %s",strerror(errno));
		while (access("/tmp/CORRELATORSLDFifo",R_OK)==-1){
	    		logMessage(stderr,__FUNCTION__,__LINE__,
					"waiting for fifo");
	    		usleep(FIFOWAITPERIOD);
		}
	}
	struct timeval to={1,0};
	size_t lastAttacks=0;
	while (true){
		PRDALERT *prdAlert=0;
		PDDALERT *pddAlert=0;
		SLDALERT *sldAlert=0;
		fd_set rfds;
		FD_ZERO(&rfds); //NOLINT(hicpp-no-assembler,readability-isolate-declaration)
		FD_SET(pddFd,&rfds); //NOLINT(hicpp-signed-bitwise)
		FD_SET(prdFd,&rfds); //NOLINT(hicpp-signed-bitwise)
		FD_SET(sldFd,&rfds); //NOLINT(hicpp-signed-bitwise)
		int nfds=max(sldFd,prdFd);
		nfds=max(nfds,pddFd)+1;
		errno=0;
		int s=select(nfds,&rfds,0,0,&to);
		if (s<0){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"select: %d %s",s,strerror(errno));
			continue;
		}
		if ((s == 0) || ((to.tv_sec==0) && (to.tv_usec==0))){
			fprintf(stdout, "%10lu %10lu",
				time(0),ATTACKS);
			fprintf(stdout,"\033[0K");
			for(size_t i=0;i<currentAttackNameX;++i){
				fprintf(stdout, " %s",currentAttackName[i]);
			}
			fprintf(stdout, "\n");
			fprintf(stdout, "Guilty IPs:% 10ld",guiltyIPs);
			fprintf(stdout,"\033[1F");
			fflush(stdout);
			fprintf(gFile,"%lu\n",ATTACKS-lastAttacks);
			fflush(gFile);
			if ((ATTACKS-lastAttacks)==0){
			       	currentAttackNameX=0;}
			lastAttacks=ATTACKS;
			to.tv_sec=1;
			to.tv_usec=0;
			continue;
		}
		if (s > 3){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"select: too many fds %d\n",s);
			continue;
		}
		if (FD_ISSET(sldFd,&rfds)){ //NOLINT(hicpp-signed-bitwise)
			++ALERTSIN;
			int rc=readFromFifo(sldFd,(void **)&sldAlert);
			if (rc==-1){
				logMessage(stderr,__FUNCTION__,__LINE__,
					"input fifo closed, stopping");
				break;
			}
			if (rc != 0) {
				logMessage(stderr,__FUNCTION__,__LINE__,
					"readFifoFailed");
				continue;
			}
			processSld(sldAlert);
			free(sldAlert);
			fflush(stderr);
			continue;
		}
		if (FD_ISSET(pddFd,&rfds)){ //NOLINT(hicpp-signed-bitwise)
			++ALERTSIN;
			int rc=readFromFifo(pddFd,(void **)&pddAlert);
			if (rc==-1){
				logMessage(stderr,__FUNCTION__,__LINE__,
					"input fifo closed, stopping");
				break;
			}
			if (rc != 0) {
				logMessage(stderr,__FUNCTION__,__LINE__,
					"readFifoFailed");
				continue;
			}
			processPdd(pddAlert);
			free(pddAlert);
			fflush(stderr);
			continue;
		}
		if (FD_ISSET(prdFd,&rfds)){ //NOLINT(hicpp-signed-bitwise)
			++ALERTSIN;
			int rc=readFromFifo(prdFd,(void **)&prdAlert);
			if (rc==-1){
				logMessage(stderr,__FUNCTION__,__LINE__,
					"input fifo closed, stopping");
				break;
			}
			if (rc != 0) {
				logMessage(stderr,__FUNCTION__,__LINE__,
					"readFifoFailed");
				continue;
			}
			processPrd(prdAlert);
			free(prdAlert);
			fflush(stderr);
			continue;
		}
		if ((!FD_ISSET(pddFd,&rfds)) || (!FD_ISSET(prdFd,&rfds))){ //NOLINT(hicpp-signed-bitwise)
			logMessage(stderr,__FUNCTION__,__LINE__,
					"select fired on unknown fd\n");
			continue;
		}
	}
	close(pddFd);
	close(prdFd);
}
