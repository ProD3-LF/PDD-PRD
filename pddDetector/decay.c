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
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "decay.h"
void initDecayData(DqDecayData *data){
	data->value = 0.0;
	data->lastUpdate = 0;
}
void initDecayAvg(DqDecayAvg *davg, DqDecayData *data, double halfLife){
	davg->data = data;
	davg->decayFactor = exp(log(0.5)/halfLife);
}
double decayAvgApplyDecay(DqDecayAvg *davg,long long time){
	double newVal;
	DqDecayData *data = davg->data;
	double age = (double)time - (double)data->lastUpdate;
//	age = age/10000000.0;
	if (age < 0){
		fprintf(stderr,"%s.%d backward time %lld=%g< %g\n",__FUNCTION__,__LINE__,time,(double)time,data->lastUpdate);
		time = data->lastUpdate;
		age = 0;
	}
	if (age > 0) {
		newVal = data->value * pow(davg->decayFactor,age);
	} else {
		newVal = data->value;
	}
	return newVal;
}
double decayAvgApplyDecayAndUpdate(DqDecayAvg *davg,long long time){
	DqDecayData *data = davg->data;
	double age = (double)time - (double)data->lastUpdate;
//	age = age/10000000.0;
	if (age < 0){
		fprintf(stderr,"%s.%d backward time %lld=%f< %f %f\n",__FUNCTION__,__LINE__,time,(double)time,data->lastUpdate,age);
		time = data->lastUpdate;
	}
	if (time != data->lastUpdate) {
		data ->value = decayAvgApplyDecay(davg,time);
		data->lastUpdate = (double)time;
	}
	return data->value;
}

void decayAvgNewObservation(DqDecayAvg *davg, double value, long long time){
	decayAvgApplyDecayAndUpdate(davg,time);
	davg->data->value += value;
}
double decayAvgNormalized(DqDecayAvg *davg,long long time){
	return decayAvgApplyDecayAndUpdate(davg,time) * (1-davg->decayFactor);
}
