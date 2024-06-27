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
#ifndef PD3_PDD_LOG_H
#define PD3_PDD_LOG_H
#include <stdint.h>
#include <stdlib.h>
#include <zlog.h>
#define ZLOG_LEVEL_TRACE  (10)
#define ZLOG_LEVEL_METRIC (30)
#ifndef PD3_LOG_H_EXTERN
#define PD3_LOG_H_EXTERN extern
#endif
PD3_LOG_H_EXTERN zlog_category_t *PDD_main;
#define zc_me (PDD_main)
#include <pd3_zlog.h>
int initPddLogging();
#endif
