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

CC = gcc

tcpdumpSensor: tcpdumpSensor.o sensorUtil.o parseTcp.o queue.o ../common/config.o ../common/pddObs.h ../common/logMessage.o ../common/util.o
	$(CC) $(CFLAGS) -static tcpdumpSensor.o -o tcpdumpSensor sensorUtil.o parseTcp.o queue.o ../common/config.o ../common/logMessage.o ../common/util.o

../common/logMessage.o: ../common/logMessage.c
	cd ../common && $(CC) $(CFLAGS) -c ../common/logMessage.c

../common/util.o: ../common/util.c ../common/config.h
	cd ../common && $(CC) $(CFLAGS) -c ../common/util.c

../common/config.o: ../common/config.c ../common/config.h
	cd ../common && $(CC) $(CFLAGS) -c ../common/config.c

tcpdumpSensor.o: tcpdumpSensor.c defs.h ../common/logMessage.h ../common/util.h ../common/pddObs.h
	$(CC) $(CFLAGS) -c tcpdumpSensor.c

sensorUtil.o: sensorUtil.c defs.h ../common/prdObs.h ../common/logMessage.h ../common/config.h ../common/pddObs.h
	$(CC) $(CFLAGS) -c sensorUtil.c

parseTcp.o: parseTcp.c defs.h ../common/pddObs.h ../common/logMessage.h
	$(CC) $(CFLAGS) -c parseTcp.c

queue.o: queue.c bucket.h defs.h ../common/prdObs.h ../common/logMessage.h
	$(CC) $(CFLAGS) -c queue.c

clean:
	rm -f tcpdumpSensor *.o ../common/*.o

