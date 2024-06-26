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
#include <limits.h>
#include <stdlib.h>
int32_t strToPI(char *s){
	char *e=0;
	long int t=strtol(s,&e,0);
	if ((t==LONG_MIN) || (t==LONG_MAX) || (e==s)){
		t=-1;
	}
	if ((t < 0) || (t > INT_MAX)){
		t=-1;
	}
	return(t);
}
