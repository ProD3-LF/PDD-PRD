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
#ifndef PDDFILEDEFS_H
#define PDDFILEDEFS_H
#include "../common/fileDefs.h"
#define MASTERTRAINFILE "/tmp/pddMasterTrain"
#define OBSUNCOLLECTED CENTRAL"/observations/uncollected"
#define OBSDIR OBSUNCOLLECTED"/%s/PDD/%lu"
#define OBSCOLLECTED CENTRAL"/observations/collected"
#define OBSSTORED OBSCOLLECTED"/%s/PDD"
#define MODELDIR CENTRAL"/models"
#define PDDMODELDIR MODELDIR"/PDD"
#define CWMAPFILEOBS OBSDIR"/cwmap.obs"
#define TCPSTARTMODELS PDDMODELDIR"/TCPSTARTMODEL"
#define TCPMODELS PDDMODELDIR"/TCPMODEL"
#define SEQSIZES "1 2 3 4 5 6"
#endif
