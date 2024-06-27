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
MODELDIR=../../../artifacts/central/models/PRD
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

.PHONY: clean prdDepend getPrdParameter buildCorModelDepend buildZeroModelDepend buildStdModelDepend

LDLIBS += -lpthread
LDLIBS += -lm
prdLib = libpd3_prdDetector.so
offline = buildCorModel buildStdModel buildZeroModel getPrdParameter
CC = gcc


all:	$(prdLib) tstPrd $(offline)

tstObjects = tstPrd.o
tstObjects += prdAlert.o
tstSources = $(tstObjects:.o=.c)

prdObjects = prdDetector.o
prdObjects += processTcp.o
prdObjects += prdAlert.o
prdObjects += prdUtil.o
prdObjects += model.o
prdObjects += ../common/flags.o
prdObjects += ../common/logMessage.o 
prdObjects += ../common/config.o
prdObjects += ../common/util.o

prdSources = $(prdObjects:.o=.c)

getPrdParameterObjects = getPrdParameter.o
getPrdParameterSources = $(getPrdParameterObjects:.o=.c)

buildCorModelObjects = buildCorModel.o
buildCorModelObjects += ../common/flags.o
buildCorModelObjects += ../common/logMessage.o 

buildCorModelSources = $(buildCorModelObjects:.o=.c)
buildStdModelObjects = buildStdModel.o
buildStdModelSources = $(buildStdModelObjects:.o=.c)
buildZeroModelObjects = buildZeroModel.o
buildZeroModelObjects += ../common/flags.o
buildZeroModelObjects += ../common/logMessage.o 

buildZeroModelSources = $(buildZeroModelObjects:.o=.c)

$(prdLib): $(prdObjects)
	$(CC) $(CFLAGS) -o $(prdLib) $(prdObjects) -shared

default: tstDepend prdDepend $(prdLib)
	prdDepend: .prdDepend

buildStdModelDepend: .buildStdModelDepend
.buildStdModelDepend: $(buildStdModelSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

buildZeroModelDepend: .buildZeroModelDepend
.buildZeroModelDepend: $(buildZeroModelSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

getPrdParameterDepend: .getPrdParameterDepend
.getPrdParameterDepend: $(getPrdParameterSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

buildCorModelDepend: .buildCorModelDepend
.buildCorModelDepend: $(buildCorModelSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

tstDepend: .tstDepend
.tstDepend: $(tstSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

prdDepend: .prdDepend
.prdDepend: $(prdSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"


include .prdDepend
include .tstDepend
include .getPrdParameterDepend
include .buildCorModelDepend
include .buildZeroModelDepend
include .buildStdModelDepend

buildStdModel: $(buildStdModelObjects)
	$(CC) $(CFLAGS) $(buildStdModelObjects) -lm -o "$@"

buildZeroModel: $(buildZeroModelObjects)
	$(CC) $(CFLAGS) $(buildZeroModelObjects) -lm -o "$@"

getPrdParameter: $(getPrdParameterObjects)
	$(CC) $(CFLAGS) $(getPrdParameterObjects) -lm -o "$@"

buildCorModel: $(buildCorModelObjects)
	$(CC) $(CFLAGS) $(buildCorModelObjects) -lm -o "$@"

tstPrd: $(tstObjects)
	$(CC) $(CFLAGS) $(tstObjects) -L. -lpd3_prdDetector $(LDLIBS) -o "$@"

clean:
	rm -f .prdDepend 
	rm -f .tstDepend 
	rm -f .getPrdParameterDepend 
	rm -f .buildCorModelDepend 
	rm -f .buildZeroModelDepend 
	rm -f .buildStdModelDepend 
	rm -f .depend 
	rm -f tstPrd
	rm -f $(prdLib)
	rm -f buildCorModel buildStdModel buildZeroModel getPrdParameter
	rm -f *.o
	rm -f ../common/*.o

# %.c: %.py
# 	$(CYTHONIZE) -i $<
