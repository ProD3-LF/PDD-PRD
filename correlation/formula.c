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
#include "../common/decay.h"
#include "../common/logMessage.h"
#include "baseProg.h"
#include "correlatorFileDefs.h"
#include "defs.h"
#include "formula.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
char GENERIC_FORMULA_FILE[PATH_MAX]=GENERIC_FORMULA_FILE_DEFAULT;
extern int AGETHRESHOLD;
extern char *extractMiraiDomains(char *s);
formula genericFormula[MAX_GENERIC_FORMULA];
size_t genericFormulaX=0;


int alertType2Class(int a){
	switch(a){
	case PDDALIEN:
	case PDDANOM:
	case PDDSTARTSHORT:
	case PDDSTARTALIEN:
	case PDDSTARTANOM:
	case PDDOVERFLOW:
	case PRDZERO:
	case PRDALLMAX:
	case PRDALLSD2:
	case PRDMAX:
	case PRDALIEN:
	case  PRDSD2:
	case  SLDTIMEOK:
	case  SLDTIMEMAXLOW:
	case  SLDTIMELOW:
	case  SLDTIMEHIGH:
	case  SLDTIMEMAXHIGH:
	case  SLDTIMEZERO:
	case  SLDCOUNTOK:
	case  SLDCOUNTMAXLOW:
	case  SLDCOUNTLOW:
	case  SLDCOUNTHIGH:
	case  SLDCOUNTMAXHIGH:
	case  SLDCOUNTZERO:
		return(PM);
	default:
		logMessage(stderr,__FUNCTION__,__LINE__,
			"bad type %d\n",a);
	}
	return(UM);
}
const char *alertType2String(int a){
	switch(a){
		case PDDALIEN: return("PDDALIEN");
		case PDDANOM: return("PDDANOM");
        	case PDDSTARTSHORT: return("PDDSTARTSHORT");
        	case PDDSTARTALIEN: return("PDDSTARTALIEN");
        	case PDDSTARTANOM: return("PDDSTARTANOM");
        	case PDDOVERFLOW: return("PDDOVERFLOW");
        	case PRDZERO: return("PRDZERO");
        	case PRDALLMAX: return("PRDALLMAX");
        	case PRDALLSD2: return("PRDALLSD2");
        	case PRDSD2: return("PRDSD2");
        	case PRDMAX: return("PRDMAX");
        	case PRDALIEN: return("PRDALIEN");
	        case SLDTIMEOK: return("SLDTIMEOK");
                case SLDTIMEMAXLOW: return("SLDTIMEMAXLOW");
                case SLDTIMELOW: return("SLDTIMELOW");
                case SLDTIMEHIGH: return("SLDTIMEHIGH");
                case SLDTIMEMAXHIGH: return("SLDTIMEMAXHIGH");
                case SLDTIMEZERO: return("SLDTIMEZERO");
                case SLDCOUNTOK: return("SLDCOUNTOK");
                case SLDCOUNTMAXLOW: return("SLDCOUNTMAXLOW");
                case SLDCOUNTLOW: return("SLDCOUNTLOW");
                case SLDCOUNTHIGH: return("SLDCOUNTHIGH");
                case SLDCOUNTMAXHIGH: return("SLDCOUNTMAXHIGH");
                case SLDCOUNTZERO: return("SLDCOUNTZERO");
		default: return("UNKNOWN");
	}
	return("UNKNOWN");
}
int alertType2Int(char *b){
	if (strncmp(b,"SLDCOUNTMAXHIGH",sizeof("SLDCOUNTMAXHIGH")-1)==0){
		return(SLDCOUNTMAXHIGH);
	}
	if (strncmp(b,"SLDTIMEMAXHIGH",sizeof("SLDTIMEMAXHIGH")-1)==0){
	       	return(SLDTIMEMAXHIGH);
	}
	if (strncmp(b,"SLDCOUNTMAXLOW",sizeof("SLDCOUNTMAXLOW")-1)==0){
	
	       	return(SLDCOUNTMAXLOW);
	}
	if (strncmp(b,"SLDTIMEMAXLOW",sizeof("SLDTIMEMAXLOW")-1)==0){
	      	return(SLDTIMEMAXLOW);
	}
	if (strncmp(b,"PDDSTARTSHORT",sizeof("PDDSTARTSHORT")-1)==0){
	    	return(PDDSTARTSHORT);
	}
	if (strncmp(b,"PDDSTARTALIEN",sizeof("PDDSTARTALIEN")-1)==0){
	    	return(PDDSTARTALIEN);
	}
	if (strncmp(b,"SLDCOUNTHIGH",sizeof("SLDCOUNTHIGH")-1)==0){
	     	return(SLDCOUNTHIGH);
	}
	if (strncmp(b,"SLDCOUNTZERO",sizeof("SLDCOUNTZERO")-1)==0){
	     	return(SLDCOUNTZERO);
	}
	if (strncmp(b,"SLDCOUNTLOW",sizeof("SLDCOUNTLOW")-1)==0){
	    	return(SLDCOUNTLOW);
	}
	if (strncmp(b,"SLDTIMEHIGH",sizeof("SLDTIMEHIGH")-1)==0){
	    	return(SLDTIMEHIGH);
	}
	if (strncmp(b,"SLDTIMEZERO",sizeof("SLDTIMEZERO")-1)==0){
	    	return(SLDTIMEZERO);
	}
	if (strncmp(b,"PDDSTARTANOM",sizeof("PDDSTARTANOM")-1)==0){
	   	return(PDDSTARTANOM);
	}
	if (strncmp(b,"PDDOVERFLOW",sizeof("PDDOVERFLOW")-1)==0){
	  	return(PDDOVERFLOW);
	}
	if (strncmp(b,"SLDTIMELOW",sizeof("SLDTIMELOW")-1)==0){
	   	return(SLDTIMELOW);
	}
	if (strncmp(b,"SLDCOUNTOK",sizeof("SLDCOUNTOK")-1)==0){
	   	return(SLDCOUNTOK);
	}
	if (strncmp(b,"SLDTIMEOK",sizeof("SLDTIMEOK")-1)==0){
	  	return(SLDTIMEOK);
	}
	if (strncmp(b,"PRDALLMAX",sizeof("PRDALLMAX")-1)==0){
       		return(PRDALLMAX);
	}
	if (strncmp(b,"PRDALLSD2",sizeof("PRDALLSD2")-1)==0){
       		return(PRDALLSD2);
	}
	if (strncmp(b,"PDDALIEN",sizeof("PDDALIEN")-1)==0){
      		return(PDDALIEN);
	}
	if (strncmp(b,"PRDALIEN",sizeof("PRDALIEN")-1)==0){
      		return(PRDALIEN);
	}
	if (strncmp(b,"PDDANOM",sizeof("PDDANOM")-1)==0){
     		return(PDDANOM);
	}
	if (strncmp(b,"PRDZERO",sizeof("PRDZERO")-1)==0){
     		return(PRDZERO);
	}
	if (strncmp(b,"PRDMAX",sizeof("PRDMAX")-1)==0){
    		return(PRDMAX);
	}
	if (strncmp(b,"PRDSD2",sizeof("PRDSD2")-1)==0){
    		return(PRDSD2);
	}
	logMessage(stderr,__FUNCTION__,__LINE__,"bad type %s\n",b);
	return(-1);
}
double alertType2Age(baseProg *b,int alertType,time_t now){
	return((double)now-b->avg[alertType].lastUpdate);
}
double alertType2Rate(baseProg *b,int alertType){
        DqDecayAvg avg;
        avg.data = &b->avg[alertType];
        avg.decayFactor=HALFLIFE;
        decayAvgNormalized(&avg,time(0));
	return(b->avg[alertType].value);
}
double alertType2Threshold(baseProg *b,int alertType){
	return(b->threshold[alertType]);
}
void initGenericFormula() {
	for(int i=0;i<MAX_GENERIC_FORMULA;++i){
		genericFormula[i].name_[0]='\0';
		genericFormula[i].nTerms_=0;
		for(int j=0;j<MAX_FORMULA_TERMS;++j){
			genericFormula[i].alertType_[j]=0;
			genericFormula[i].threshold_[j]=0;
			genericFormula[i].coefficient_[j]=0;
		}
	}
}
void classifyAda(baseProg *b){
	time_t now=time(0);
	b->nrm=b->hrm=b->pm=b->um=0;
	for(int i=0;i<MAX_ALERT_TYPES;++i){
		time_t a=(time_t)alertType2Age(b,i,now);
		if (a>AGETHRESHOLD){ continue; }
		double r=alertType2Rate(b,i);
		if (r < 0){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"bad rate\n");
			continue;
		}
		switch(alertType2Class(i)){
		case NRM: b->nrm+=r;break;
		case HRM: b->hrm+=r;break;
		case PM: b->pm+=r;break;
		case UM: b->um+=r;break;
		}
	}
	double t=b->nrm+b->hrm+b->pm+b->um;
	b->nrm=b->nrm/t;
	b->hrm=b->hrm/t;
	b->pm=b->pm/t;
	b->um=b->um/t;
}
double evaluateFormula(baseProg *b,formula *f,time_t now){
	if (f->nTerms_ == 0){return(0);}
	double score=0;
	for(int i=0;i<f->nTerms_;++i){
		time_t a=(time_t)alertType2Age(b,f->alertType_[i],now);
		if (a>AGETHRESHOLD){ return(0); }
	}
	for(int i=0;i<f->nTerms_;++i){
		double r=alertType2Rate(b,f->alertType_[i]);
		if (r < 0){
			logMessage(stderr,__FUNCTION__,__LINE__,
					"bad rate\n");
			return(0);
		}
		double t=0;
		if (f->threshold_[i] == -1){
			t = alertType2Threshold(b,f->alertType_[i]) *
				f->coefficient_[i];
		} else {t = f->threshold_[i];}
		if (r > t){score +=1;}
		else{score+=r/t;}
	}
	score = score/(double)f->nTerms_;
	return(score);
}
int loadTerm(char *b,formula *f){
	size_t n=strnlen(b,LONGSTRING);
	if (n >= LONGSTRING){
		logMessage(stderr,__FUNCTION__,__LINE__, 
				"input formula too long");
		return(0);
	}
	if ((f->alertType_[f->nTerms_]=alertType2Int(b))==-1){
		logMessage(stderr,__FUNCTION__,__LINE__,
				"invalid alert type %s\n",b);
		return(0);
	}
	size_t i=0;
	for(i=0;i<n;++i){
		if (isspace(b[i])){
			break;
		}
	}
	if (i == n){
		fprintf(stderr,"%s.%d no first space\n",
			__FUNCTION__,__LINE__);
		return(0);
	}
	char *e=0;
	char *s=&b[i+1];
	f->threshold_[f->nTerms_] = strtod(s,&e);
	if (e == s) {
		logMessage(stderr,__FUNCTION__,__LINE__,
				"ERROR threshold strtod\n");
		return(0);
	}
	s = e;
	f->coefficient_[f->nTerms_] = strtod(s,&e);
	if (e == s) {
		fprintf(stderr,"%s.%d ERROR coefficient strtod\n",
			__FUNCTION__,__LINE__);
		return(0);
	}
	f->nTerms_++;
	return(1);
}
size_t readGenericFormulas(){
	char buf[LONGSTRING];
	int state=0;
	FILE *f=fopen(GENERIC_FORMULA_FILE,"re");
	if (f==0){
		fprintf(stderr,"%s.%d ERROR fopen(%s) %s\n",
			__FUNCTION__,__LINE__,GENERIC_FORMULA_FILE,
					strerror(errno));
		return(0);
	}
	while(fgets(buf,LONGSTRING,f) != 0){
		if (buf[0] == '#'){continue;}
		if (buf[0] == '\n'){continue;}
		if (state == 0) {
			if (sscanf(buf,
			     "%s {\n",genericFormula[genericFormulaX].name_)!=1){
				fprintf(stderr,"%s.%d ERROR parse in %s\n",
					__FUNCTION__,__LINE__,buf);
				

			} else{state = 1;}
		} else {
			if (state == 1) {
				if (buf[0] == '}'){
					++genericFormulaX;
					state=0;
				}
				else {
					loadTerm(buf,
			     		     &genericFormula[genericFormulaX]);
				}
			}
		}
	}
	if (state != 0){
		fprintf(stderr,"%s.%d ERROR incomplete formula\n",
			__FUNCTION__,__LINE__);
	}
	fclose(f);
	return(genericFormulaX);
}

size_t evaluateGenericFormulas(baseProg *b,time_t now,double score[]){
	size_t i=0;
	for(;i<genericFormulaX;++i){
		score[i]=evaluateFormula(b,&genericFormula[i],now);
	}
	return(i);
}
void initGenericFormulas(){
	readGenericFormulas();
}
