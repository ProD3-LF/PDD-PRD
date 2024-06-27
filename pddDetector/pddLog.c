#/* SPDX-License-Identifier: Apache-2.0 */
#/* Copyright (c) 2012-2023 Applied Communication Sciences
# * (now Peraton Labs Inc.)
# *
# * This software was developed in work supported by the following U.S.
# * Government contracts:
# *
# * HR0011-20-C-0160 HR0011-16-C-0061
# * 
# *
# * Any opinions, findings and conclusions or recommendations expressed in
# * this material are those of the author(s) and do not necessarily reflect
# * the views, either expressed or implied, of the U.S. Government.
# *
# * DoD Distribution Statement A
# * Approved for Public Release, Distribution Unlimited
# *
# * DISTAR Case 38846, cleared November 1, 2023
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# * http://www.apache.org/licenses/LICENSE-2.0
# */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// define here, so declarations happen here.
#define PD3_LOG_H_EXTERN
#include "pddLog.h"

struct {
    zlog_category_t **zc;
    char *name;
}
LogCategories[] = {
    // put all logging categories here.
    {&PDD_main, "PDD_main"},
};
extern int  pd3_init_zlog();
int initPddLogging(char *fname) {
    unsigned int i;
    int rc;
    rc = pd3_init_zlog();
    if (rc) {
	fprintf(stderr, "%s.%d zlog init failed: %d <%s>\n",
			__FUNCTION__,__LINE__,rc, fname);
	return(-1);
    }
    fprintf(stderr, "%s.%d zlog init worked: %d <%s>\n",
			__FUNCTION__,__LINE__,rc, fname);
    for (i = 0; i < sizeof(LogCategories)/sizeof(LogCategories[0]); i++) {
	zlog_category_t *zc;
	char *name = LogCategories[i].name;
	zc = zlog_get_category(name);
	if (zc == NULL) {
	    fprintf(stderr, "zlog_get_category(\"logging\") failed.");
	    return -1;
	}
	*LogCategories[i].zc = zc;
    }
    return 0;
}
