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
#include "../common/fileDefs.h"
#include "prdFileDefs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int main(int argc, char *argv[]){
	if (argc != 2){
		fprintf(stderr,"%s.%d usage: %s variable\n",
				__FUNCTION__,__LINE__,argv[0]);
		exit(-1);
	}
	if (strcmp(argv[1],"CENTRAL")==0){
		fprintf(stdout,"%s",CENTRAL);
		exit(0);
	}
	if (strcmp(argv[1],"OBSDIR")==0){
		fprintf(stdout,"%s",OBSDIR);
		exit(0);
	}
	if (strcmp(argv[1],"TMPWORKINGDIR")==0){
		fprintf(stdout,"%s",TMPWORKINGDIR);
		exit(0);
	}
	if (strcmp(argv[1],"OBSCOLLECTED")==0){
		fprintf(stdout,"%s",OBSCOLLECTED);
		exit(0);
	}
	if (strcmp(argv[1],"OBSUNCOLLECTED")==0){
		fprintf(stdout,"%s",OBSUNCOLLECTED);
		exit(0);
	}
	if (strcmp(argv[1],"PRDOBSFILE")==0){
		fprintf(stdout,"%s",PRDOBSFILE);
		exit(0);
	}
	if (strcmp(argv[1],"CWMAPFILEOBS")==0){
		fprintf(stdout,"%s",CWMAPFILEOBS);
		exit(0);
	}
	if (strcmp(argv[1],"OBSSTORED")==0){
		fprintf(stdout,"%s",OBSSTORED);
		exit(0);
	}
	if (strcmp(argv[1],"STOREDMODELDIR")==0){
		fprintf(stdout,"%s",STOREDMODELDIR);
		exit(0);
	}
	if (strcmp(argv[1],"STOREDCORCOFFILE")==0){
		fprintf(stdout,"%s",STOREDCORCOFFILE);
		exit(0);
	}
	if (strcmp(argv[1],"STOREDSTDMODELFILE")==0){
		fprintf(stdout,"%s",STOREDSTDMODELFILE);
		exit(0);
	}
	if (strcmp(argv[1],"STOREDCWMAPFILE")==0){
		fprintf(stdout,"%s",STOREDCWMAPFILE);
		exit(0);
	}
	if (strcmp(argv[1],"STOREDZPROBFILE")==0){
		fprintf(stdout,"%s",STOREDZPROBFILE);
		exit(0);
	}
	if (strcmp(argv[1],"COMMONMODELDIR")==0){
		fprintf(stdout,"%s",COMMONMODELDIR);
		exit(0);
	}
	if (strcmp(argv[1],"MODELDIR")==0){
		fprintf(stdout,"%s",MODELDIR);
		exit(0);
	}
	if (strcmp(argv[1],"CWMAPFILE")==0){
		fprintf(stdout,"%s",CWMAPFILE);
		exit(0);
	}
	fprintf(stderr,"%s.%d %s is not a prd variable\n",
			__FUNCTION__,__LINE__,argv[1]);
}

