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
This directory (pddDetector) and the parallel directory ../common
contain the source code for the Protocol Deviant Detector (PDD).

------------------------------OVERVIEW-----------------------------------------
PDD looks for anomalous use of TCP with respect to sequences of
TCP flags within individual TCP sessions.

PDD examines the TCP flags in all TCP packets to/from the
protected server, which is identified by IP and PORT (see configuration
section).

PDD's input is a pddObs data structure (../common/pddObs.h) which contains
a set of observables, each comprising:
 	clientIP, clientPort, serverIP, serverPort, flags, and direction.

PDD processes each observable by adding the flag and direction to the
sequence for the session defined by clientIP, clientPort, serverIP, 
and serverPort, and then checking the resulting sequence against a learned
set of sequences.  Sequences of specific lengths (e.g. 1, 2, 4, 6, 10) are
checked.

PDD's primary output is an alert, which indicates an anomaly was found.  PDD
also periodic outputs statistical information in the form of an ssv file (which
helps you to know it is working in the typical case when there are no alerts,
and is useful for forensics). 

--------------------------------INPUT-----------------------------------------
The pddObs input is provided by another component called the sensor. The
sensor is not part of PDD, and it is expected that end users will write their
own sensor using their choice of packet capture technology.  Nevertheless, an
example sensor using tcpdump for packet capture is provided (in ../sensor).
Since PDD must see all packets for a session the packet capture technology
must not drop any packets.  Unfortunately tcpdump may drop packets so the
example sensor may cause problems.  (See the readme in ../sensor for more
about this.)

The sensor captures packets, and builds the pddObs structure. 
Perioducally, or when the pddObs structure is full the sensor sends
the pddObs to PDD over a set of fifos (/tmp/PDDFifo%d).  The number of fifos
is configurable.  Having more fifos improves performance (to a limit) as
it allows more parallelism, however the following rule applies to fifos:
	the pddObs structure sent over fifoN must contain observations for
	which client port % N = 0
This rule ensures that no sessions span fifos and therefore PDD
need not coordinate across pddObs from different fifos.  Because of this
parallism in simple; PDD can have a thread for each fifo.
(Sensor trivially implements the rule: it concurrently constructs N pddObs
loading pddObs[i] only with obs whose client port % i = 0, and then sends
pddObs[i] over fifoi when pddObs[i] is full.)

PDD abstractly considers the flag and direction in the pddObs as a
Control Word (CW), which is a positive or negative integer.  The numeric
part of the CW is a simple mapping (../central/common/cwmap) between the
flag and an arbirarily assigned integer.  The sign comes from the direction:
if the direction is incoming (to the protected server) then the sign is
negative, if the direction is outgoing (from the protected server) then the
sign is positive.

For each unique session PDD maintains a pddDetector data structure which
records the static information about the session, and maintains enough of
the sequence to support checking for the maximum length being used.  
A set of index tables (serverData & sessionData) allows the pddDetector object
for a specific session to be rapidly found based on the client and server IP
and Port.

-------------------------------------MODELS-----------------------------------
The models define the non-anomalous CW sequences.  Models are stored
in ../central/models/PDD.  There are two classes of model: start and ongoing.
The start models are used for the begining sequence of a TCP session
since this is where anomalies due to attack usually show up.  The ongoing
models are used after the current sequence exceeds the length of the longest
start model sequence.  The models are stored in files named
	TCPSTARTMODEL.N and
	TPCMODEL.N,
where N is the length of the sequences conatined in that file.
These files are ASCII with each line containing N space seperate integers
representing a non-anomalous sequence of size N.  The file is sorted in
numerical order. 

PDD maintains a count of total CW recieved for each session.  If that count
is less than N, PDD uses the TCPSTARTMODELs, otherwise PDD uses the TCPMODELs.

The models are produced by running PDD detector in record mode to collect
observations and then running model building tools to create/update the models.

The lengths of the sequences to be checked is configurable.  
See the section on TRAINING for specific details.

Weakness in current implementation:  PDD assumes it starts BEFORE any
TCP session that it is monitoring began, thus PDD always sees the begining
of each TCP session.  If PDD starts while a TCP session is ongoing PDD will
incorrectly use the start sequences for the ongoing session and detect
anomalies.

-----------------------------------ANOMALY TYPES-------------------------------
PDD categorizes anomalies as:
	ALIEN:	a CW that does not appear in an ongoing model (this might
		be a prohibited combination of flags like SYNFIN, or
		it maybe an extremely rare combination seen for the
		first time.
	START_SHORT:  during the start of a TCP session the sequence did
		not progress out of the start state for longer than a 
		time limit (configuable to exceed TCP TIMEOUT).
	START_ALIEN: A CW that does not appear in a start model while
		the TCP session is starting
	START_ANOM: a CW sequence that is not in start models while the
		TCP session is starting
	ANOM: a CW sequence that is not in the ongoing models after the
		TCP session has started
	OVERFLOW: an internal error condition.  PDD input queues have become
		full so input has been lost.  When an overflow occurs
		subsequent alerts may be suspect since data has been lost.
		Alerts occuring within the time period TCP TIMEOUT after
		the last OVERFLOW alert are unreliable.
------------------------------CONFIGURATION-----------------------------------
The ../common/config file contains two run-time configuration parameters:
	pddmode: [detect|record|detect-record], and
	pddservers: IP:PORT
The pddmode specifies whether PDD will detect anomalies (normal operation),
record observations for training, or both.
pddservers is a list of space seperated protected servers using dotted
decimal notation for IP and decimal port number (e.g. 10.1.1.1:80).
NOTE: config file allows specification of multiple protected server but PDD
	currently supports only one.  Multiple protected servers is for 
	future enhancement.

PDD uses files to store models and observations.  These files are configured
through ../common/fileDefs.h and pddFileDefs.h  The settings in the files
should be adequate for most users.  (In all description of files used in this
README, the default settings are assumed.)

Hardcoded configuration variables (#defines in .h files) to be aware of are:
MAXPORTS: the largest port number possible (set to 65536)
NSERVD: the maximum number of protected servers (set to 1)
	NOTE: FOR FUTURE USE, ONLY ONE PROTECTED SERVER CAN BE SPECIFIED.
MAXSCVEC: The longest length CW sequence to be considered (currently 20)
TCPIDLETIME: The TCP time out period.  Sessions are considered abandoned if PDD
received no observables for this period (currently 650000 msec).
NFIFOS: the number of FIFOs to use for communication of pddObs from sensor
	to PDD.
MAXOBS: the maximum number of observations in a pddObs.
MAXOBSTIME: the maximum time (in msec) to wait before sending a pddObs.
NOTE: MAXOBS and MAXOBSTIME work together; sensor outputs a pddObs either 
when it is full or when it is not empty and MAXOBSTIME has elapsed since it
was last output.

It is not expected that users will need to change any of these variables.
-----------------------------------SAMPLING------------------------------------
PDD needs to see all packets associated with each session in order to
properly detect anomalies, but under high load this may not be possible.
Under high load the sensor and PDD can use a sampling technique.  The sensor
detects the need for sampling (e.g. because output queues are becoming full)
and institutes sampling using the sample element of the pddObs.  Essentially,
sensor only adds observations to a pddObs if sample % clientPort = 0.  Normaly
sample is 1, so this calculation has no effect.  Under heavy load however sensor
set sample to values > 1, which reduces sensor output observation rate.  For
example setting sample to 2 reduces the output rate by 2.  PDD MUST KNOW
WHEN THE SAMPLE VALUE SENSOR IS USING CHANGES, because PDD must know that
those ongoing sessions which have become sampled out are exempt from
anomaly detection, otherwise interruption/restoral of a sequence as a result of
sampling could appear as anomalous.  PDD handles this by
	maintaining a count of uninterrupted CWs in pddDetector for each session
	setting uninterrupted CW count to 0 when sampling that would
		effect the session
	skipping anomaly detection if uninterrupted CW count is less
		than length of sequence being checked.
Obviously when a session is sampled out there will be no anomalies detected
for that session because PDD will receive no input for that session, however
when the overflow condition resolves and sampling is reduced, PDD may
begin recieving input for a previously sampled out session.  The uninterrupted
CW count logic prevents these restarted sessions for appearing anomalous.

--------------------------------------TRAINING-------------------------------
Training is the process of having PDD record observations related to
anomalies and then post-processing these observations to incorporate them
into the current models. The TCP sessions used for training must be
attack-free, otherwise the CW sequences associated with attacks would be
learned as normal.

When PDD is in either record or detect-record mode (see configuration section)
any time PDD detects an anomaly PDD records that anomaly to a file.  The
file name is define as MASTERTRAINFILE in pddFileDefs.h.  Because of
PDD's parallelism there is one file for each of PDD's threads, which is
determined by the NFIFOs configuration variable. (Having one file per
thread eliminates concurrency issues.) 

After PDD has recorded some observations the MASTERTRAINFILES are incorporated
into the current model using these steps:
0) ./firstTime THIS IS ONLY DONE THE FIRST TIME WHEN THERE ARE NO EXISTING
	MODELS.  It creates empty PDD models with very old timestamps.
1) ./convertMasterTraining
	this converts the observations in MASTERTRAINFILEs into a 
	a more suitable form and organizes them into a timestamped
	directory structure
		../central/observations/uncollected/HOSTNAME/PDD/TIMESTAMP
	where HOSTNAME is the name of the computer running PDD, and
	TIMESTAMP is a unique timestamp in unix epoch time format.
2) ./collectObs
	this promotes the observations produced by convertMasterTraining
	into the paralle ../central/observations/collected directory.
3) ./buildPddModels -h HOSTNAME -a TCP
	this processes all the observations in
		../central/observations/collected/HOSTNAME/PDD
	and produces new models (in ../central/model/PDD).
While these steps could be combined, keeping them discrete allows for 
simpler error recovery should the user determine that a set of observations
should not be processed.
Note that buildPddModels archives the prior model in
	../central/model/PDD.TIMESTAMP
therefore reverting to an older model is simple, just replace
	../central/model/PDD with ../central/model/PDD.TIMESTAMP
To speed up step 3, buildPddModels uses an incremental approach: only 
collected observations which are new than the current model are considered.

PDD learns what sequence lengths to use based on the content of
	../central/models/PDD.
PDD examines this directory and determins the individual N values based on
the numeric suffixes of the TCPMODEL and TCPSTARTMODEL files.  ./firstTime
(used to initialize ../central/models/PDD) creates empty files with 
n=1, 2, 4, 6, and 10.

In the event that a problem with some collected set of observations 
(stored in ../central/collected/HOSTNAME/PDD/TIMESTAMP) is found (e.g. it
accidently contains an attack sequence), that TIMESTAMP directory can be
deleted, and ./firstTime rerun.  Thereafter steps 1 - 3 will build models
based only on the remaining timestamp directories, essentailly eliminating
any sequences only in the deleted directory.
---------------------------RUNNING--------------------------------
PDD is built as a C library.  It is started by invoking the function
pddMain() from a user-provided C program.  pddMain() takes one argument,
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
PDD allows the user to specify user-specific functions for handling of
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
extern  void sendOverFlow();
	called when an overflow condition is detected.
extern  void sendAlert(uint32_t serverIP,uint32_t serverPort,
                        uint32_t clientIP, uint16_t clientPort,
                        enum SCalert type,int v[],int n);
	called when an anomaly is detected.  First four arguments 
	identify the session. type is the type of anomaly as described above
	n is the lenght of the anomalous sequence, v gives each of the n
	CWs in the anomalous sequence.

As an example, a simple alert reporting mechanism that writes alert information
to a file is implemented in pddAlert.c, which has working example of
each callback function (initAlert, flushAlerts,closeAlert,sendOverFlowAlert,
and sendPddAlert, respectively).

When starting PDD through the pddDetectorMain() function, these callbacks are
passed using the callBacks structure, defined in pdd_seq_detector.h.
----------------------STARTUP ANOMALIES-----------------------------
PDD assumes that it is started before any traffic to the protected server
occurs.  If PDD is started while this traffic is ongoing, then PDD will
incorrectly detect STARTANOMALIES for the ongoing traffic.  This defficiency
may be removed in later releases.

-----------------------FILE ORGAINIZATION----------------------------
tstPdd.c
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
