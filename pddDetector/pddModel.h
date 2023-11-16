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
#ifndef PDDMODEL_H
#define PDDMODEL_H
#include "defs.h"
typedef enum {
  SC_POL_OK,					/*sc seq is good*/
  SC_POL_ERROR,					/*Error in the policy*/
  SC_POL_ANOMALOUS,				/*sc seq is anomalous*/
  SC_POL_ALIEN,					/*sc seq contains an alien*/
} ScStatusCode;
#pragma pack(push,4)
struct scVector_
{
  int freq;
  time_t lastSeen; 
  int sc[];
} typedef scVector;
struct scModel_
{
  int n;
  scVector v[];
} typedef scModel;
#define MAXSCVEC 20
extern scModel *TCPMODEL[MAXSCVEC+1];
extern scModel *TCPSTARTMODEL[MAXSCVEC+1];
#pragma pack(pop)
extern ScStatusCode checkSc(int v[], int n,char *appName, scModel ** sc, int oldSc, int *isEditAnomaly);
extern void putScModel(scModel * sc);
extern void SCMCACHE_INIT(void);
extern void SCMCACHE_DESTROY(void);

extern char *scModelDir;
struct scModelMap_ {
	char appName[SHORTSTRING];
	scModel *m;
} typedef scModelMap;
extern struct dictionary_header seq_model_list;
extern scVector* scVectorAt(scVector *v0,int size,int pos);

#endif



/*
  Local Variables:
  mode:c
  c-file-style:"GNU"
  c-backslash-max-column:160
  c-file-offsets:((brace-list-open . 0)(substatement-open . 0))
  comment-column:48
  comment-end:""
  comment-start:"// "
  End:
*/
/* vim: sw=2
 */
