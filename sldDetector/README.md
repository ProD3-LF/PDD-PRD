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
# * DISTAR Case 39809, cleared May 22, 2024
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# * http://www.apache.org/licenses/LICENSE-2.0
# */
This directory (sldDetector) and the parallel directory ../common
contain the source code for the Session Lifecycle Deviant Detector (SLD).

------------------------------OVERVIEW-----------------------------------------
SLD looks for anomalous TCP session durations
SLD monitors all TCP packets to the protected server,
which is identified by IP and PORT (see configuration section).
SLD monitors each session to determine when active periods begin
and end.  An active period begins when a packet occurs, and ends
when the last packet for the period occurs.  Occurance of the
last packet is detected when no packets have been recieved for a 
"long" time.
SLD in detect-record mode collects observations of the activity period
in terms of time and number of packets seen.  Then an off-line proceedure
builds a model of typical behavior.  Finally, SLD in detect mode uses
the model to detect anomalies.
The model for both the count and the duration is charactorized by
	LOWMAX (the lowest value seen)
	LOW (a value such that only 3% of values seen are lower)
	HIGH (a value such that only 3% of values seen are higher)
	HIGH MAX (the highest value seen)
	ZERO (a special case when only one packet occurs and duration
		cannot be calculated).
While running in detect mode, SLD reports alerts when a session violates
one of the above charactorizations using the names listed.
SLD prepends the word TIME or COUNT to the alert name to indicate if
the anomaly involved activity period duration, or activity period 
packet count.

SLD seems most helpful in detecting very short activity periods, which
typically arise when spoofed client IP addresses are used to create
sessions, since these spoofed clients will not respond to any packets
sent to them by the server.

SLD's input is a sldObs data structure (../common/sldObs.h)
which contains the server IP and port, client IP and port, and timestamp
of each packet.
This input is supplied by a sensor (see sensor section).

SLD's output is an alert, which indicates an anomaly was found.

--------------------------------INPUT-----------------------------------------
The sldObs input is provided by another component called the sensor. The
sensor is not part of SLD, and it is expected that end users will write their
own sensor using their choice of packet capture technology.  Nevertheless, an
example sensor using tcpdump for packet capture is provided (in ../sensor).

The sensor captures packets, and builds the SLD structure. 
At the expiration of a time window or when a large number of samples
have been collected, the sensor sends
the sldObs to SLD over a fifo (/tmp/SLDFifo).

-------------------------------------MODELS-----------------------------------
A typical SLD model looks like this
	TIME03: 0.129087
	TIME97: 118.143439
	TIME0: 0.000072
	TIME100: 146.199690
	COUNT03: 36.000000
	COUNT97: 2337.000000
	COUNT0: 2.000000
	COUNT100: 4074.000000

Those items named TIME refer to durations, while those labeled COUNT refer
to packet counts.   The number after the count label refers to the percentage
point.  For example TIME03 is the value for which 3% of samples are lower,
TIME97 is the value for which 3% of samples are higher, TIME0 is the lowest
sample, and TIME100 is the highest sample.

The models are produced by running SLD detector in record mode to collect
observations and then running model building tools to create/update the models.


-----------------------------------ANOMALY TYPES-------------------------------
SLD categorizes anomalies as:
	SLDTIMELOWMAX a value less than TIME0 was seen
	SLDTIMELOW a value less than TIME03 was seen
	SLDTIMEHIGHMAX a value greater than TIME97 was seen
	SLDTIMEHIGH a value created than TIME100 was seen
	SLDCOUNTLOWMAX a value less than COUNT0 was seen
	SLDCOUNTLOW a value less than COUNT03 was seen
	SLDCOUNTHIGHMAX a value greater than COUNT97 was seen
	SLDCOUNTHIGH a value created than COUNT100 was seen
	SLDTIMEZERO an activity period with only 1 packet was seen

------------------------------CONFIGURATION-----------------------------------
The ../common/config file contains two run-time configuration parameters:
	sldmode: [detect|record|detect-record], and
	sldservers: IP:PORT
The sldmode specifies whether SLD will detect anomalies (normal operation),
record observations for training, or both.
sldservers is a list of space seperated protected servers using dotted
decimal notation for IP and decimal port number (e.g. 10.1.1.1:80).
NOTE: config file allows specification of multiple protected server but SLD
	currently supports only one.  Multiple protected servers is for 
	future enhancement.

SLD uses files to store models and observations.  These files are configured
through ../common/fileDefs.h and sldFileDefs.h  The settings in the files
should be adequate for most users.  (In all description of files used in this
README, the default settings are assumed.)

Hardcoded configuration variables (#defines in .h files) to be aware of are:

It is not expected that users will need to change any of these variables.
--------------------------------------TRAINING-------------------------------
Training is the process of having SLD record observations related to
anomalies and then post-processing these observations to incorporate them
into the current models. The TCP sessions used for training must be
attack-free, otherwise the session characteristics associated with attacks
would be learned as normal.

When SLD is in either record or detect-record mode (see configuration section)
SLD will output the value of the intput sldObs observation to a unique file.
The file is stored in
   ../central/observations/uncollected/HOSTNAME/SLD/STARTIME/sldObs.STARTTIME
HOSTNAME is the name of the computer on which SLD is running.  STARTTIME
is the time at which SLD started.

After a set of observations has been collected (and SLD stopped) the models
are produced by running:
	1) ./collectObs (moves observations from uncollected to collected
		directory (i.e. ../central/observations/collected/HOSTNAME/SLD/STARTTIME)
	2) ./buildSldModels.sh -h HOSTNAME (builds a new set of models from
		the set of collected observations.  Multiple hosts can be
		specified.
The new models are stored in ../central/models/SLD, with a copy of the prior
models stored in ../central/models/PRD.TIMESTAMP.
Note that in the event that bad models are created (e.g. an attack accidently
occured during observation collection), the offending TIMESTAMP directory can
be removed and the ./buildSldModels.sh program re-run to build a set of models
with out the offending observations.

---------------------------RUNNING--------------------------------
SLD is built as a C library.  It is started by invoking the function
sldMain() from a user-provided C program.  sldMain() takes one argument,
the CALLBACK structure, in which the user supplies user-written functions
for handling SLD alerts.
---------------------------LOGGING-------------------------------
A simple API is used for logging:
logMessage(FILE *S,char *F,int L,VNSPINTF STYLE ARGS);
S is the open stread you want the log message to go to (e.g. stderr)
F and L are a string and integer that will appear on the output line
	using __FUNCTION__ for f and __LINE__ for L make sense
	the rest of the args are just like printf.
For example logMessage(stderr,__FUNCTION__,__LINE__,"THIS IS MESSAGE %d",10);
---------------------------CALLBACKS-------------------------------
SLD allows the user to specify user-specific functions for handling of
alerts.  The user must provide the following functions:

extern void init();
	called once when SLD starts: use to initializes your alert reporting
	method (e.g. open a file to which alerts will be written)
extern  void flush();
	called periodically when processing observations: use to perform any
	routine alert related activity (e.g. flush file to which alerts are
	being written)
extern  void close();
	call once when SLD stops: use to terminate your alert reporting
	method (e.g. close file to which alerts were written)
extern sendAlert(uint32_t serverIP,uint16_t serverPort, char *violation);
	called when an anomaly is detected.  First two arguments 
	identify the server. violation give a string identifying the
	anomaly type (see Anomaly Types section).

As an example, a simple alert reporting mechanism that writes alert information
to a file is implemented in sldAlert.c, which has working example of
each callback function (initAlert, flushAlerts,closeAlert,sendOverFlowAlert,
and sendPddAlert, respectively).

When starting SLD through the sldDetectorMain() function, these callbacks are
passed using the callBacks structure, defined in defs.h.

-----------------------FILE ORGAINIZATION----------------------------
tstSld.c:	defines main, setsup callback, invokes sld detector
sldDetector.c:	top-level detector code
model.c:	code related to dealing with the models
sldUtil.c;	general utility functions
sldLog.c:	logging
sldAlert.c:	alert reporting

buildDensityModel.c: off line model builder
defs.h:		general definitions
sldDetector.h:	structures used for detection
sldLog.h:	logging
sldModel.h:	structures for the model
getSldParameter.c: off-line program for accessing #defines from shell program
sldAlert.h:	alert structures
sldFileDefs.h	defines related to file structure (observations and models)
sldAlertTypes.h:defines related to alert names
meanStdDev.c:	not used, but maybe in future conside mean and stddev models
