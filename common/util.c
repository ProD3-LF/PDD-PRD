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
#include "logMessage.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#define MSECPERSEC 1000
#define USECPERSEC 1000000
long long unsigned int msecTime(){
        struct timespec t;
        clock_gettime(CLOCK_REALTIME,&t);
        return(t.tv_sec*MSECPERSEC+t.tv_nsec/USECPERSEC);
}
#define DIRMODE 0777
int mkdirEach(char *path){
	size_t n=strlen(path);
	char part[PATH_MAX];
	size_t k=0;
	for(size_t i=0;i<n;++i){
		if ((part[k++]=path[i])=='/'){
			part[k]='\0';
			if (mkdir(part,DIRMODE)!=0){
				if (errno != EEXIST){
		       			return(-1);
				}
			}
		}
	}
	part[k]='\0';
	if (mkdir(part,DIRMODE)!=0){
		if (errno != EEXIST){
		       	return(-1);
		}
	}
	return(0);
}
