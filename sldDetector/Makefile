#/* SPDX-License-Identifier: Apache-2.0 */
#/* Copyright (c) 2012-2023 Applied Communication Sciences
#* (now Peraton Labs Inc.)
#*
#* This software was developed in work supported by the following U.S.
#* Government contracts:
##*
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
MODELDIR=../../../artifacts/central/models/SLD
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

.PHONY: clean sldDepend buildDensityModel tidy

LDLIBS += -lpthread
LDLIBS += -lm
sldLib = libpd3_sldDetector.so
offline = buildDensityModel getSldParameter
CC = gcc


all:	$(sldLib) tstSld $(offline) tidy

tstObjects = tstSld.o
#tstObjects += sendAlert.o
tstSources = $(tstObjects:.o=.c)

sldObjects = sldDetector.o
sldObjects += sldUtil.o
sldObjects += sldAlert.o
sldObjects += model.o
sldObjects += ../common/logMessage.o 
sldObjects += ../common/config.o
sldObjects += ../common/util.o
sldObjects += ../common/decay.o
sldSources = $(sldObjects:.o=.c)

buildDensityModelObjects += buildDensityModel.o
buildDensityModelObjects += meanStdDev.o
buildDensityModelObjects += ../common/util.o
buildDensityModelSources = $(buildDensityModelObjects:.o=.c)

getSldParameterObjects = getSldParameter.o
getSldParameterSources = $(getSldParameterObjects:.o=.c)

$(sldLib): $(sldObjects)
	$(CC) $(CFLAGS) -o $(sldLib) $(sldObjects) -shared

default: tstDepend sldDepend $(sldLib) getSldParameter tidy
	sldDepend: .sldDepend

tstDepend: .tstDepend
.tstDepend: $(tstSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

sldDepend: .sldDepend
.sldDepend: $(sldSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

getSldParameterDepend: .getSldParameterDepend
.getSldParameterDepend: $(getSldParameterSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"



buildDensityModelDepend: .buildDensityModelDepend
.buildDensityModelDepend: $(buildDensityModelSources)
	rm -f "$@"
	$(CC) $(CFLAGS) -MM $^ > "$@"

include .sldDepend
include .tstDepend
include .getSldParameterDepend

buildDensityModel: $(buildDensityModelObjects)
	$(CC) $(CFLAGS) $(buildDensityModelObjects) -lm -o "$@"

tstSld: $(tstObjects)
	$(CC) $(CFLAGS) $(tstObjects) -L. -lpd3_sldDetector $(LDLIBS) -o "$@"

clean:
	rm -f .sldDepend 
	rm -f .tstDepend 
	rm -f .buildDensityModelDepend 
	rm -f getSldParameter
	rm -f .getSldParameterDepend
	rm -f .depend 
	rm -f tstSld
	rm -f buildDensityModel
	rm -f $(sldLib)
	rm -f *.o
	rm -f ../common/*.o
	rm -f $(DIST_LIB)/$(sldLib)
	rm -f $(DIST_BIN)/tstSld
install: all
	cp tstSld $(DIST_BIN)
	cp $(sldLib)  $(DIST_LIB)
	mkdir -p $(DIST_CONF)/central/models

../common/util.o:       ../common/util.c ../common/util.h
	gcc $(DEFINES) $(CFLAGS) $(FLAGS) $(INCLUDES) -c $< -o $@

../common/config.o:     ../common/config.c
	gcc $(DEFINES) $(CFLAGS) $(FLAGS) $(INCLUDES) -c $< -o $@

../common/logMessage.o: ../common/logMessage.c
	gcc $(DEFINES) $(CFLAGS) $(FLAGS) $(INCLUDES) -c $< -o $@

../common/flags.o:   ../common/flags.c ../common/flags.h
	gcc $(DEFINES) $(CFLAGS) $(FLAGS) $(INCLUDES) -c $< -o $@

define runclang
	clang-tidy -checks=*,-llvm-header-guard,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling -header-filter=.*\.h $(1) > $1.tidy;
endef

tidy: 
	$(foreach s,$(sldSources),$(call runclang,$(s)))
	$(foreach s,$(tstSources),$(call runclang,$(s)))
	$(foreach s,$(buildDensityModelSources),$(call runclang,$(s)))
	$(foreach s,$(getSldParameterSources),$(call runclang,$(s)))



# %.c: %.py
# 	$(CYTHONIZE) -i $<
