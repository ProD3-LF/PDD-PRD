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
#define LOGFILEDIR "/tmp"
#include "../common/fileDefs.h"
#define OBSUNCOLLECTED CENTRAL"/observations/uncollected"
#define OBSCOLLECTED CENTRAL"/observations/collected"
#define OBSCOLLECTEDCORRELATOR OBSCOLLECTED"/*/correlator/[0-9][0-9]*"
#define MODELDIR CENTRAL"/models"
#define CORRELATORMODELDIR MODELDIR"/correlator"
#define ALERTSRATEDIR OBSCOLLECTED"/%s/correlator/%lu"
#define ARMODELDIR MODELDIR"/correlator/current/AR"
#define ARMODELFILE_DEFAULT ARMODELDIR"/ar"
#define GENERIC_FORMULA_FILE_DEFAULT CORRELATORMODELDIR"/formula"
#define GENERIC_SCENARIO_FILE_DEFAULT CORRELATORMODELDIR"/scenario"
