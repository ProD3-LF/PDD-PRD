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
#include "defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int parseLine(char *b,size_t n,char *appName,int *alertType,int *score){
	size_t l = strnlen(b,n);
	while(l > 0) {
		if ((b[l] == ' ') || (b[l] == '\t')){break;}
		--l;
	}
	if (l == 0){
		fprintf(stderr,"%s.%d ERROR cannot parse %s\n",
				__FUNCTION__,__LINE__,b);
		return(-1);
	}
	*score = strToPI(&b[l+1]);
	if (*score == -1) {
		fprintf(stderr,"%s.%d ERROR cannot parse %s\n",
				__FUNCTION__,__LINE__,b);
		return(-1);
	}
	--l;
	while(l > 0) {
		if ((b[l] == ' ') || (b[l] == '\t')){break;}
		--l;
	}
	if (l == 0){
		fprintf(stderr,"%s.%d ERROR cannot parse %s\n",
				__FUNCTION__,__LINE__,b);
		return(-1);
	}
	*alertType = strToPI(&b[l+1]);
	if (*alertType == -1){
		fprintf(stderr,"%s.%d ERROR cannot parse %s\n",
				__FUNCTION__,__LINE__,b);
		return(-1);
	}
	for(size_t i=0;i<l;++i){appName[i]=b[i];}
	appName[l]='\0';
	return(0);
}
