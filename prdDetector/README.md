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
This directory (prdDetector) and the parallel directory ../common
contain the source code for the Protocol Deviant Detector (PDD).

------------------------------OVERVIEW-----------------------------------------
PRD looks for anomalous use of TCP with respect to the ratio between
TCP flags across concurrent sessions.
PRD monitors all TCP packets to the protected server, which is identified by
IP and PORT (see configuration section).
PRD extracts the TCPFLAGS and direction for each TCP packet and combines
then into a value know as a Control Word (CW).
PRD maintains counts of the number of times each CW occurred during
a time window (30 seconds).  At the end of the time window PRD computes the
pairwise ratio between:
`	1) the counts of every pair of CWs
`	2) the counts of each CW and the total number of CWs.
PRD then compares these ratios to the corresponding ratio in a set of models.
The model records the mean, stddev, and maximum value for each ratio.  PRD
issues alerts if the ratio being checked exceeds the maxiumum or 2 stddevs from
the mean in the models.

Models are produced by running PRD in record mode which caused PRD to output
the counts after each time window to a set of files.  These files are then
post-processed to computer the mean, max, and stddev (see Training section).

Recognizing that some CWs are completely indepedent of other CWs, the post
processing does correlation analsis to produce a corelation coeffient for each
pair.  PRD skips comparison of CW pairs having a low correlation coeffient,
Since some CW counts may have a value of 0 during a given time window, and
ratios cannot be computed when one value is 0, PRD also uses a zero probability
model which gives the probability that the count of CW A is zero given
that CW B has value X.  (X is bucketized into high, medium, and low, based on
the distribution of its recorded values.)

PRD's input is a nodeCwCnt data structure (../common/prdObs.h)
the count of each control work for a time window.  This input is supplied
by a sensor (see sensor section).

PRD's primary output is an alert, which indicates an anomaly was found.  PRD
also periodic outputs statistical information in the form of an ssv file (which
helps you to know it is working in the typical case when there are no alerts,
and is useful for forensics). 

--------------------------------INPUT-----------------------------------------
The nodeCwCnt input is provided by another component called the sensor. The
sensor is not part of PRD, and it is expected that end users will write their
own sensor using their choice of packet capture technology.  Nevertheless, an
example sensor using tcpdump for packet capture is provided (in ../sensor).

The sensor captures packets, and builds the nodeCwCnt structure. 
At the expiration of a time window, the sensor sends
the nodeCwCnt to PRD over a fifo (/tmp/PRDFifo).

PRD abstractly considers the flag and direction in the nodeCwCnt as a
Control Word (CW), which is a positive or negative integer.  The numeric
part of the CW is a simple mapping (../central/common/cwmap) between the
flag and an arbirarily assigned integer.  The sign comes from the direction:
if the direction is incoming (to the protected server) then the sign is
negative, if the direction is outgoing (from the protected server) then the
sign is positive.

Since PRD works on a time window, at startup PRD cannot begin to check for
anomalies immediately, PRD must wait until at least a time window's worth
of observations has arrived before it can start checking for anomalies.

-------------------------------------MODELS-----------------------------------
The models define the non-anomalous CW pair ratios sequences.  Models are stored
in ../central/models/PRD. 
	The pairwise ratios are store in PRDSTDMODEL.
	The correlation coefficients are stored in corcor.
	The zero probability models are stored in ZPROB.

The models are produced by running PRD detector in record mode to collect
observations and then running model building tools to create/update the models.


-----------------------------------ANOMALY TYPES-------------------------------
PDD categorizes anomalies as:
	PRDZERO: violation of the zero probability model
	PRDALLMAX: ratio between a CW count and the total CW count exceeded
		maximum model value.
	PRDALLST2:  ratio between a CW cound and the total CW count exceeded
		2 stddevs of the mean model value.
	PRDMAX: ratio between a pair of CW counts exceeded
		maximum model value.
	PRDSD2:  ratio between a pair of CW counts exceeded
		2 stddevs of the mean model value.
Since it has been found that transient conditions may cause an occasional 
false positive, PRD only outputs an alert when the anomalous condition is
present for at least two time windows.

------------------------------CONFIGURATION-----------------------------------
The ../common/config file contains two run-time configuration parameters:
	prdmode: [detect|record|detect-record], and
	prdservers: IP:PORT
The prdmode specifies whether PDD will detect anomalies (normal operation),
record observations for training, or both.
prdservers is a list of space seperated protected servers using dotted
decimal notation for IP and decimal port number (e.g. 10.1.1.1:80).
NOTE: config file allows specification of multiple protected server but PRD
	currently supports only one.  Multiple protected servers is for 
	future enhancement.

PRD uses files to store models and observations.  These files are configured
through ../common/fileDefs.h and prdFileDefs.h  The settings in the files
should be adequate for most users.  (In all description of files used in this
README, the default settings are assumed.)

Hardcoded configuration variables (#defines in .h files) to be aware of are:

It is not expected that users will need to change any of these variables.
--------------------------------------TRAINING-------------------------------
Training is the process of having PRD record observations related to
anomalies and then post-processing these observations to incorporate them
into the current models. The TCP sessions used for training must be
attack-free, otherwise the CW ratios associated with attacks would be
learned as normal.

When PRD is in either record or detect-record mode (see configuration section)
PRD will output the value of the intput nodeCwCnt observation to a unique file
every time window.
The file is stored in
   ../central/observations/uncollected/HOSTNAME/PRD/STARTIME/prdObs.WINDOWTIME
HOSTNAME is the name of the computer on which PRD is running.  STARTTIME
is the time at which PRD started, WINDOWTIME is the time of each unique window.
Note that with in the STARTTIME directory there will be many prdObs.WINDOWTIME
files, one for each time window.

After a set of observations has been collected (and PRD stopped) the models
are produced by running:
	1) ./collectObs (moves observations from uncollected to collected
		directory (i.e. ../central/observations/collected/HOSTNAME/PRD/STARTTIME)
	2) ./buildPrdModels.sh -h HOSTNAME (builds a new set of models from
		the set of collected observations.  Multiple hosts can be
		specified.
The new models are stored in ../central/models/PRD, with a copy of the prior
models stored in ../central/models/PRD.TIMESTAMP.
Note that in the event that bad models are created (e.g. an attack accidently
occured during observation collection), the offending TIMESTAMP directory can
be removed and the ./buildPrdModels.sh program re-run to build a set of models
with out the offending observations.

---------------------------RUNNING--------------------------------
PRD is built as a C library.  It is started by invoking the function
prdMain() from a user-provided C program.  prdMain() takes one argument,
the CALLBACK structure, in which the user supplies user-written functions
for handling PDD alerts.
---------------------------LOGGING-------------------------------
A simple API is used for logging:
logMessage(FILE *S,char *F,int L,VNSPINTF STYLE ARGS);
S is the open stread you want the log message to go to (e.g. stderr)
F and L are a string and integer that will appear on the output line
	using __FUNCTION__ for f and __LINE__ for L make sense
	the rest of the args are just like printf.
For example logMessage(stderr,__FUNCTION__,__LINE__,"THIS IS MESSAGE %d",10);
---------------------------CALLBACKS-------------------------------
PRD allows the user to specify user-specific functions for handling of
alerts.  The user must provide the following functions:

extern void init();
	called once when PDD starts: use to initializes your alert reporting
	method (e.g. open a file to which alerts will be written)
extern  void flush();
	called periodically when processing observations: use to perform any
	routine alert related activity (e.g. flush file to which alerts are
	being written)
extern  void close();
	call once when PDD stops: use to terminate your alert reporting
	method (e.g. close file to which alerts were written)
extern sendAlert(uint32_t serverIP,uint16_t serverPort, char *violation);
	called when an anomaly is detected.  First two arguments 
	identify the server. violation give a string identifying the
	anomaly type (see Anomaly Types section).

As an example, a simple alert reporting mechanism that writes alert information
to a file is implemented in prdAlert.c, which has working example of
each callback function (initAlert, flushAlerts,closeAlert,sendOverFlowAlert,
and sendPddAlert, respectively).

When starting PRD through the prdDetectorMain() function, these callbacks are
passed using the callBacks structure, defined in pdd_seq_detector.h.

-----------------------FILE ORGAINIZATION----------------------------
tstPrd.c
	an example program providing a main() and showing how to
	start PPD by invoking the library
pddDetector.c
	entry point for stating PDD in the library
	performs initialization
	listens to FIFOs from sensor to get pddObs
	adds incoming pddObs to internal Q
	unloads pddObs from internal Q and invoke processTcp to handle
processTcp.c
	processes the pddObs data structure from sensor by
	invoking processObs for each observation in pddObs
	finds existing or adds new session to index tables
	then invokes processCW to add the new CW to the session
processCW.c
	adds the new CW to the session and analyzes the resulting new
	sequence for anomalies by checking against models
ttPDD.c
	routines for checking a sequence of CW against the models
model.c
	routines for initializating PDD internal representation of
	models from the model files
pdd_seq_detector.c
	routines related to pddDetector structure which is used to
	monitor each session.
pddAlert.c
	an example of callbacks for handling alerts
ppdUtil.c
	various routines for configuration and initialization
convertMasterTraining.c
	for off line training: converts the masterTrainingFiles produced
	by PDD and copies them to the uncollected observation directory
pddModel.c
	for off line training: processes the new files in the collected
	observations directory and incorporates them into the PDD models
defs.h
	definition of things common to most files
model.h &  pddModel.h
	definition of things related to model
pddFileDefs.h
	definition of the locations for storing PDD files
pdd_seq_detector.h
	definition of pddDector structure
