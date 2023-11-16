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
#include "../common/logMessage.h"
#include "pddModel.h"
#include <stdio.h>
#include <time.h>
static int compareVSv(const int *v1, scVector * v2,int ng) {
	for(int i=0;i<ng;++i){
		if (v1[i] < v2->sc[i]){return (-1);}
		if (v1[i] > v2->sc[i]){return (1);}
	}
	return (0);
}
static ScStatusCode checkVScModel(scModel * sc,int vn, int *vfull, int ng) {
  int s=0,m=0,e=0,rc=0;
  int *v=&vfull[vn-ng];
  if (sc->n == 0) {
      return (SC_POL_ANOMALOUS);
  }
  e = sc->n - 1;
  while (s <= e) {
    m = (s + e) / 2;
    scVector *vm = scVectorAt(&sc->v[0],ng,m);
    if ((rc = compareVSv(v,vm,ng)) == 0) {
      return (SC_POL_OK);
    }
    if (rc < 0) { e = m - 1; }
    else { s = m + 1; }
  }
  if (ng == 1){return(SC_POL_ALIEN);}
  return (SC_POL_ANOMALOUS);
}
ScStatusCode checkVStart(int v[],int n,scModel ** sc,int ng) {
  return(checkVScModel(*sc,n,v,ng));
}
ScStatusCode checkVSc(int v[],int n,scModel **sc,int ng) {
  if (v[0] == 0){return(SC_POL_OK);}
  return(checkVScModel(*sc,n,v,ng));
}
