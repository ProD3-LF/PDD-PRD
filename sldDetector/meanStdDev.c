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
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
long double dabs(double d){
	if (d < 0){return(-1*d);}
	return(d);
}
long double square(long double n){
	return(n*n);
}
void meanStdDev(double *s,int n, double *m, long double *stddev,double *max,
		double *maxV,double *minV){
	int c=0;
	long double stddevd=0;
	long double sum=0;
	*maxV=0;
	*minV=DBL_MAX;
	*m=0;
	*max=0;
	for(int i=0;i<n;++i){
		if (s[i]>0){
			sum += s[i];
			++c;
		}
		if (s[i] > *maxV){*maxV=s[i];}
		if (s[i] < *minV){*minV=s[i];}
	}
	if (c==0) {
		*m=-1;
		*stddev=0;
		return;
	}
	if (sum == 0) {
		fprintf(stderr,"%s.%d m is 0\n",__FUNCTION__,__LINE__);
		exit(-1);
	}
	sum = sum/c;
	if (sum == 0) {
		fprintf(stderr,"%s.%d sum is 0\n",__FUNCTION__,__LINE__);
		exit(-1);
	}
	if (sum < 0){
		fprintf(stderr,"%s.%d bad sum %Lg\n",__FUNCTION__,__LINE__,sum);
		sum=0;
		for(size_t i=0;i<n;++i){
			if (s[i]>0){
				sum += s[i];
				fprintf(stderr,"%s.%d %lu %g %Lg\n",
					__FUNCTION__,__LINE__,i,s[i],sum);
			}
			if (sum < 0){
				fprintf(stderr,"%s.%d BAD AT %lu %g %Lg\n",
					__FUNCTION__,__LINE__,i,s[i],sum);
				 exit(-1);
			}
		}
		exit(-1);
	}
	*m=(double)sum;
	if (c==1) {
		*stddev=0;
		return;
	}
	long double t=0;
	for (size_t i=0;i<n;++i){
		if (s[i] > 0){
			long double diff = dabs(s[i] - *m);
			long double d=square(diff);
			if (d < 0){
				fprintf(stderr,"%s.%d bad sqare %Lg\n",
					__FUNCTION__,__LINE__,*stddev);
				exit(-1);
			}
			t += d;
			if (t < 0){
				fprintf(stderr,"%s.%d %Lf %g %Lf\n",
					__FUNCTION__,__LINE__,t,*m,d);
				exit(-1);
			}
			if (diff > *max){*max = (double)diff;}
		}
	}
	stddevd = sqrt((double)(t/(c-1)));
	//*stddev = ceil((double)stddevd);
	*stddev = stddevd;
	if (*stddev < 0){
		fprintf(stderr,"%s.%d bad stddev %Lg\n",__FUNCTION__,__LINE__,*stddev);
		exit(-1);
	}
	if (*m < 0){
		fprintf(stderr,"%s.%d bad mean %g\n",__FUNCTION__,__LINE__,*m);
		exit(-1);
	}
}	
