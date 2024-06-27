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
#include "flags.h"
#include "fileDefs.h"
#include "logMessage.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define VFMT " %80[^{},\n\t]"
#define LONGSTRING 512
int flagToCW[MAXCW2];
int CWMAPN=0;
int CURRENTCONTROLWORD=0;
cwMap CWMAP[MAXCW];
int inRequestedRange(int start,int stop,int k){
	int minusStart=-stop+MAXCW-1;
	int minusStop=-start+MAXCW+1;
	int plusStart=start+MAXCW-1;
	int plusStop=stop+MAXCW+1;
	if (((k>minusStart)&&(k<minusStop))||
		((k>plusStart)&&(k<plusStop))){return(0);}
	return(1);
}
char *flagsAsString(uint8_t f) {
	static char fs[LONGSTRING];
	size_t i=0;
	if ((f & URG)!=0){//NOLINT(hicpp-signed-bitwise)
	       	strncpy(&fs[i],"Urg",LONGSTRING-i);
		i+=3;
	}
	if ((f & ACK)!=0){//NOLINT(hicpp-signed-bitwise)
	       	strncpy(&fs[i],"Ack",LONGSTRING-i);
		i+=3;
	}
	if ((f & PSH)!=0){//NOLINT(hicpp-signed-bitwise)
	       	strncpy(&fs[i],"Psh",LONGSTRING-i);
		i+=3;
	}
	if ((f & RST)!=0){//NOLINT(hicpp-signed-bitwise)
	       	strncpy(&fs[i],"Rst",LONGSTRING-i);
		i+=3;
	}
	if ((f & SYN)!=0){//NOLINT(hicpp-signed-bitwise)
	       	strncpy(&fs[i],"Syn",LONGSTRING-i);
		i+=3;
	}
	if ((f & FIN)!=0){//NOLINT(hicpp-signed-bitwise)
	       	strncpy(&fs[i],"Fin",LONGSTRING-i);
	}
	return(fs);
}
unsigned char flagsToInt(char *f){
	unsigned char t=0;
	if (strstr(f,"Urg")!=0){t |= URG;}//NOLINT(hicpp-signed-bitwise)
	if (strstr(f,"Ack")!=0){t |= ACK;}//NOLINT(hicpp-signed-bitwise)
	if (strstr(f,"Psh")!=0){t |= PSH;}//NOLINT(hicpp-signed-bitwise)
	if (strstr(f,"Rst")!=0){t |= RST;}//NOLINT(hicpp-signed-bitwise)
	if (strstr(f,"Syn")!=0){t |= SYN;}//NOLINT(hicpp-signed-bitwise)
	if (strstr(f,"Fin")!=0){t |= FIN;}//NOLINT(hicpp-signed-bitwise)
	return(t);
}
void initFlagToCW() {
	for (size_t i=0;i<MAXCW2;++i){
		flagToCW[i]=0;
	}
	for (size_t i=0;i<CWMAPN;++i){
		flagToCW[flagsToInt(CWMAP[i].controlWordString)]=CWMAP[i].cw;
	}
}
int convertCwName(char *cwName){
       for (size_t i=0;i<CWMAPN;++i){
	       if (strcmp(cwName,CWMAP[i].controlWordString)==0){
		       return(CWMAP[i].cw);
	       }
       }
       return(0);
}
void convertCw(int cw,char *cws,size_t n){
       if (cw == 0) {
	       strncpy(cws,"ALL",n);
	       return;
       }
       if (cw < 0){cws[0]='-';}
       else{cws[0]='+';}
       cws[1]='\0';
       for (size_t i=0;i<CWMAPN;++i){
	       if (CWMAP[i].cw == cw){
		       strncat(&cws[1],CWMAP[i].controlWordString,n-1);
		       return;
	       }
	       if (CWMAP[i].cw == -cw){
		       strncat(&cws[1],CWMAP[i].controlWordString,n-1);
		       return;
	       }
       }
       snprintf(&cws[1],n-1,"UNK-CW-%d",abs(cw));
}
void setMinMaxCw(int *minCw,int *maxCw){
	*maxCw=0;
	*minCw=MAXCW2;
	for(size_t i=0;i<MAXCW;++i){
		if (CWMAP[i].cw > *maxCw){*maxCw = CWMAP[i].cw;}
	}
	*minCw=0 - *maxCw;
	*maxCw=MAXCW + *maxCw;
	*minCw=MAXCW + *minCw;
}

char initCwMapError[ERROR_STRING];
char *initCwMap(){
	for(int i=0;i<MAXCW;++i){
		CWMAP[i].controlWordString[0]='\0';
		CWMAP[i].cw=0;
	}
	FILE *f=fopen(CWMAPFILE,"re");
	if (f!=0){
		char b[LONGSTRING];
		while(fgets(b,LONGSTRING,f)){
			if (sscanf(b,VFMT "," "%d", //NOLINT(cert-err34-c)
				CWMAP[CWMAPN].controlWordString,&CWMAP[CWMAPN].cw)!=2){
				fclose(f);
				sprintf(initCwMapError,"%s.%d cannont parse %s/n",__FUNCTION__,__LINE__,b);
				return(initCwMapError);
			}
			if (CWMAP[CWMAPN].cw > CURRENTCONTROLWORD){
				CURRENTCONTROLWORD=CWMAP[CWMAPN].cw;
			}
			++CWMAPN;
		}
		fclose(f);
	}
	else { 
		sprintf(initCwMapError,"%s.%d fopen(%s):%s\n",
			__FUNCTION__,__LINE__,CWMAPFILE,strerror(errno));
		return(initCwMapError);
	}
	return(0);
}
