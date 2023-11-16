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
#ifndef PRDFILEDEFS_H
#define PRDFILEDEFS_H
#include "../common/fileDefs.h"
#define OBSUNCOLLECTED CENTRAL"/observations/uncollected"
#define OBSCOLLECTED CENTRAL"/observations/collected"
#define OBSDIR OBSUNCOLLECTED"/%s/PRD/%lu"
#define TMPWORKINGDIR "/tmp/observations/PRD"
#define PRDOBSFILE OBSDIR "/prdObs.%llu"
#define CWMAPFILEOBS OBSDIR"/cwmap.obs"
#define OBSSTORED CENTRAL"/%s/observations/PRD"
#define STOREDMODELDIR MODELDIR"/PRD"
#define STOREDCORCOFFILE STOREDMODELDIR"/corcof"
#define STOREDSTDMODELFILE STOREDMODELDIR"/PRDSTDMODEL"
#define STOREDCWMAPFILE COMMONMODELDIR"/cwmap"
#define STOREDZPROBFILE STOREDMODELDIR"/ZPROB"
#endif
