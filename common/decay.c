#include "decay.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define HALFLIFE 0.5
double getScore(DqDecayData *data,time_t t){
	DqDecayAvg avg;
	avg.data=data;
	avg.decayFactor=HALFLIFE;
	return(decayAvgNormalized(&avg,t));
}
double updateScore(DqDecayData *data,time_t t,int count){
	DqDecayAvg avg;
	avg.data=data;
	avg.decayFactor=HALFLIFE;
	decayAvgNewObservation(&avg,count,t);
	return(decayAvgNormalized(&avg,t));
}
void initDecayData(DqDecayData *data){
	data->value = 0.0;
	data->lastUpdate = 0;
}
void initDecayAvg(DqDecayAvg *davg, DqDecayData *data, double halfLife){
	davg->data = data;
	davg->decayFactor = exp(log(HALFLIFE)/halfLife);
}
double decayAvgApplyDecay(DqDecayAvg *davg,long long time){
	DqDecayData *data = davg->data;
	double age = (double)time - data->lastUpdate;
	if (age < 0){
		//fprintf(stderr,"%s.%d backward time %lld=%g< %g\n",__FUNCTION__,__LINE__,time,(double)time,data->lastUpdate);
		age = 0;
	}
	if (age > 0) {
		return(data->value * pow(davg->decayFactor,age));
	}
	return(data->value);
}
double decayAvgApplyDecayAndUpdate(DqDecayAvg *davg,long long time){
	DqDecayData *data = davg->data;
	double age = (double)time - data->lastUpdate;
	if (age < 0){
	//	fprintf(stderr,"%s.%d backward time %lld=%10.10f< %10.10f\n",__FUNCTION__,__LINE__,time,(double)time,data->lastUpdate);
		time = (long long)data->lastUpdate;
	}
	if (time != (long long)data->lastUpdate) {
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
