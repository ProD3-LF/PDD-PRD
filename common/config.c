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
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define MAXCONFIGLINE 512
char *get_config_string(FILE *f,char *key){
	static char b[MAXCONFIGLINE];
	char *k;
	size_t kn;
	kn=strlen(key)+1;
	if ((k=malloc(MAXCONFIGLINE))==0){
		fprintf(stderr,"%s.%d malloc failed\n",__FUNCTION__,__LINE__);
		exit(-1);
	}
	strcpy(k,key);
	strcat(k,":");
	rewind(f);
	while (fgets(b,sizeof(b),f)){
		size_t vn;
		char *vp;
		if (strncmp(b,k,kn)==0){
			vp=&b[kn+1];
			vn=strlen(vp);
			if (vp[vn-1]=='\n') {
				vn--;
				vp[vn]='\0';
			}
			strcpy(k,vp);
			return(k);
		}
	}
	free(k);
	return(0);
}
