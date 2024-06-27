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
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>

void logMessage(FILE *f,const char *func, int line, const char *format, ...) {
   va_list args;
   struct timeval tv;
   gettimeofday(&tv,0);
   fprintf(f,"%lu.%06lu %s.%d ",tv.tv_sec,tv.tv_usec,func,line);
   va_start(args, format);
   vfprintf(f,format, args);
   fprintf(f,"\n");
   va_end(args);
}
