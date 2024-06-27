#ifndef DECAY_H
#define DECAY_H
#include <time.h>
typedef struct DqDecayData_ {
	double value;
	double lastUpdate;
} DqDecayData;
typedef struct DqDecayAvg {
	double decayFactor;
	DqDecayData *data;
} DqDecayAvg;
#define HALFLIFE 0.5
extern double getScore(DqDecayData *data,time_t t);
extern double updateScore(DqDecayData *data,time_t t,int count);
extern void initDecayData(DqDecayData *data);
extern void initDecayAvg(DqDecayAvg *davg, DqDecayData *data, double halfLife);
extern double decayAvgApplyDecay(DqDecayAvg *davg,long long time);
extern double decayAvgApplyDecayAndUpdate(DqDecayAvg *davg,long long time);
extern void decayAvgNewObservation(DqDecayAvg *davg, double value, long long time);
extern double decayAvgNormalized(DqDecayAvg *davg,long long time);
#endif
