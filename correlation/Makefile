#/* SPDX-License-Identifier: Apache-2.0 */
#/* Copyright (c) 2012-2023 Applied Communication Sciences
#* (now Peraton Labs Inc.)
#*
#* This software was developed in work supported by the following U.S.
#* Government contracts:
#*
#* HR0011-20-C-0160 HR0011-16-C-0061
#*
#*
#* Any opinions, findings and conclusions or recommendations expressed in
#* this material are those of the author(s) and do not necessarily reflect
#* the views, either expressed or implied, of the U.S. Government.
#*
#* DoD Distribution Statement A
#* Approved for Public Release, Distribution Unlimited
#*
#* DISTAR Case 39809, cleared May 22, 2024 
#*
#* Licensed under the Apache License, Version 2.0 (the "License");
#* you may not use this file except in compliance with the License.
#* You may obtain a copy of the License at
#*
#* http://www.apache.org/licenses/LICENSE-2.0
#*/
CFLAGS=\
-Wno-address-of-packed-member \
-g \
-fPIC \
-Werror \
-Wall \
-march=corei7 \
-mtune=corei7-avx  \
-DCLIB_DEBUG \
-DPUBLIC \
-fstack-protector \
-DFORTIFY_SOURCE=2 \
-fno-common \
-I. \
-I.. \
-I$(DEV_INC) \
-I$(DEV_INC)/pd3_pinfo \
#-O3 \


.PHONY:	clean tstDepend correlatorDepend

LDLIBS += 
LDLIBS += -ldl
LDLIBS += -lpthread
LDLIBS += -lpq
LDLIBS += -lm

CC = gcc

correlatorLib = libpd3_correlator.so

offline = getCorrelatorParameter processAR



all:	$(correlatorLib) tstCorrelator tidy $(offline)
	echo "THIS IS ALL TFB"


CC=gcc

install: all


tstObjects = tstCorrelator.o
tstSources = $(tstObjects:.o=.c)

processARObjs =
processARObjs += processAR.o
processARObjs += loadAR.o
processARObjs += parseLine.o
processARObjs += strToPI.o

getCorrelatorParameterObjects = getCorrelatorParameter.o
getCorrelatorParameterSources = $(getCorrelatorParameterObjects:.o=.c)

correlatorObjs =
correlatorObjs += correlator.o
correlatorObjs += formula.o
correlatorObjs += ../common/flags.o
correlatorObjs += ../common/logMessage.o
correlatorObjs += ../common/config.o
correlatorObjs += scenario.o
correlatorObjs += ../common/decay.o
correlatorObjs += alertRate.o
correlatorObjs += getMyIps.o
correlatorObjs += processAlerts.o
correlatorObjs += networkEvidence.o
correlatorObjs += classifier.o
correlatorObjs += correlatorLog.o
correlatorObjs += loadAR.o
correlatorObjs += parseLine.o
correlatorObjs += strToPI.o
correlatorObjs += configureCorrelator.o

correlatorSources = $(correlatorObjs:.o=.c)

processARSources = $(processARObjs:.o=.c)

tstDepend: .tstDepend
.tstDepend: $(tstSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

getCorrelatorParameterDepend: .getCorrelatorParameterDepend
.getCorrelatorParameterDepend: $(getCorrelatorParameterSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

processARDepend: .processARDepend
.processARDepend: $(processARSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

correlatorDepend: .correlatorDepend
.correlatorDepend: $(correlatorSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

include .correlatorDepend
include .tstDepend
include .processARDepend
include .getCorrelatorParameterDepend

%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -MM -isystem $(DEV_INC) $< >$@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' <$@.$$$$ >$@; \
	rm -f $@.$$$$

getCorrelatorParameter:	$(getCorrelatorParameterObjects)
	$(CC) $(CFLAGS) $(getCorrelatorParameterObjects) -o "$@"

processAR:	$(processARObjs)
	$(CC) $(CFLAGS) $(processARObjs) -o "$@"

tstCorrelator:	$(tstObjects)
	$(CC) $(CFLAGS) $(tstObjects) -L. -lpd3_correlator $(LDLIBS) -o "$@"

$(correlatorLib): $(correlatorObjs)
	$(CC) $(CFLAGS) -o $(correlatorLib) $(correlatorObjs) -shared

define runclang
	clang-tidy -checks=*,-llvm-header-guard,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling -header-filter=.*\.h $(1) > $1.tidy;
endef
clean:
	rm -f *.d *.d.[0-9]* *.o *~ 
	rm -f tstCorrelator
	rm -f $(correlatorLib)
	rm -f .tstDepend .correlatorDepend .getCorrelatorParameterDepend .processARDepend

tidy: 
	$(foreach s,$(correlatorSources),$(call runclang,$(s)))
	$(foreach s,$(tstSources),$(call runclang,$(s)))
	$(foreach s,$(processARSources),$(call runclang,$(s)))
	$(foreach s,$(getCorrelatorParameterSources),$(call runclang,$(s)))

# vim:ts=4

