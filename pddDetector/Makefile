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
# SPDX-License-Identifier: Apache-2.0 */
# Copyright (c) 2012-2023 Applied Communication Sciences
# (now Peraton Labs Inc.)
#
# This software was developed in work supported by the following U.S.
# Government contracts:
#
# HR0011-20-C-0160 HR0011-16-C-0061
# 
#
# Any opinions, findings and conclusions or recommendations expressed in
# this material are those of the author(s) and do not necessarily reflect
# the views, either expressed or implied, of the U.S. Government.
#
# DoD Distribution Statement A
# Approved for Public Release, Distribution Unlimited
#
# DISTAR Case 38846, cleared November 1, 2023
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
CFLAGS=\
-Wno-address-of-packed-member \
-g \
-fPIC \
-Werror \
-Wall \
-march=corei7 \
-mtune=corei7-avx  \
-DCLIB_DEBUG \
-fstack-protector \
-DFORTIFY_SOURCE=2 \
-fno-common
INCLUDES = \
-I. \
-I..
#-O3 \

.PHONY: clean pddDepend getPrdParameterDepend pddModelDepend convertMasterTrainingDepend

LDLIBS += -lpthread
LDLIBS += -lm
pddLib = libpd3_pddDetector.so
offline = convertMasterTraining getPddParameter pddModel
CC = gcc


all:	$(pddLib) tstPdd $(offline)

tstObjects = tstPdd.o
tstObjects += pddAlert.o
tstSources = $(tstObjects:.o=.c)

pddModelObjects = pddModel.o
pddModelObjects += model.o
pddModelObjects += ../common/flags.o
pddModelObjects += ../common/logMessage.o

pddModelSources = $(pddModelObjects:.o=.c)

convertMasterTrainingObjects = convertMasterTraining.o
convertMasterTrainingSources = $(convertMasterTrainingObjects:.o=.c)

getPddParameterObjects = getPddParameter.o
getPddParameterSources = $(getPddParameterObjects:.o=.c)


pddObjects = model.o
pddObjects += pddDetector.o
pddObjects += processTcp.o
pddObjects += pddUtil.o
pddObjects += pdd_seq_detector.o
pddObjects += processCW.o
pddObjects += ttPDD.o
pddObjects += ../common/config.o
pddObjects += ../common/logMessage.o
pddObjects += ../common/util.o
pddObjects += ../common/flags.o

pddSources = $(pddObjects:.o=.c)

pddModel: $(pddModelObjects)
	$(CC) $(CFLAGS) $(pddModelObjects) -o $@

$(pddLib): $(pddObjects)
	$(CC) $(CFLAGS) -o $(pddLib) $(pddObjects) -shared

default: tstDepend pddDepend pddModelDepend pddModel convertMasterTrainingDepend convertMasterTraining getPddParameter $(pddLib)
	pddDepend: .pddDepend
	pddModelDepend: .pddModelDepend
	convertMasterTrainingDepend: .convertMasterTrainingDepend

tstDepend: .tstDepend
.tstDepend: $(tstSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

pddDepend: .pddDepend
.pddDepend: $(pddSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

getPddParameterDepend: .getPddParameterDepend
.getPddParameterDepend: $(getPddParameterSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

pddModelDepend: .pddModelDepend
.pddModelDepend: $(pddModelSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

convertMasterTrainingDepend: .convertMasterTrainingDepend
.convertMasterTrainingDepend: $(convertMasterTrainingSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

include .pddDepend
include .pddModelDepend
include .getPddParameterDepend
include .convertMasterTrainingDepend
include .tstDepend

tstPdd: $(tstObjects)
	$(CC) $(CFLAGS) $(tstObjects) -L. -lpd3_pddDetector $(LDLIBS) -o "$@"

clean:
	rm -f .pddDepend 
	rm -f .tstDepend 
	rm -f .convertMasterTrainingDepend 
	rm -f .depend 
	rm -f tstPdd
	rm -f .pddModelDepend 
	rm -f .getPddParameterDepend 
	rm -f $(pddLib)
	rm -f *.o
	rm -f convertMasterTraining
	rm -f ../common/*.o
	rm -f pddModel
	rm -f convertMasterTraining

# %.c: %.py
# 	$(CYTHONIZE) -i $<
