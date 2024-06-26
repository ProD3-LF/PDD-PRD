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
This directory (correlation) and the parallel directory ../common
contain the source code for the correlator

------------------------------OVERVIEW-----------------------------------------
Correlator recieves input alerts from a variety of detectors, currently
	PRD,
	PDD, and
	SLD,
and performs correlation between the alerts to deduce if they are representative
of attacks or merely false positives.
Correlation is based on human-specified criteria in three areas:
1) classification: a set of input alert corresponds to a set known to be
	associated with a specific attack.
2) scenario: an time ordered sequence of alerts corresponds to a sequence
	that is highly indicative of an attack
3) formula: an execptionally high volume of diverse alerts occuring over a
	short period of time that is somewhat indicative of an attack

Classifications are the most precise and formulas are the least precise.

Acknowledging that detectors produce false positive alerts, correlator uses
an alert-specific rate charactorization proceedure for the formulas.
--------------------------------INPUT-----------------------------------------
Correlator's input is the alert output of the SLD, PDD, and PRD detectors,
which is described in the parallel sldDetector, pddDetector, and prdDetector
directories.

-------------------------------------MODELS-----------------------------------
Correlator uses models to charactorize the typical false positive rate of
each possible input alert.  The rate is a score based on an exponential decay
formula.  An example model is:
c0a80201.80     14      27
c0a80201.80     15      26
c0a80201.80     20      21

The first column gives the server IP and port, the second gives the alertID,
and the thrid gives the threshold for that alert for the specified server.
Note that these models are niether produced or consumed by humans, so the
cryptic representation is irrelevant.  The alert ids are defined in alert.h
In the above example the first line means that the SLDTIMELOW alert hase a
threshold of 27 for server 192.168.2.1:80.  For formula analysis only, if the
score of incoming alerts of type SLDTIMELOW associated with sert 192.168.2.1:80
is less than 27, these alerts will not be considered in the formula.

------------------------------CONFIGURATION-----------------------------------
The ../common/config file contains several correlator configuration parameters
	correlator,mode: [detect|record|detect-record],
	correlator,genericScenarioFile,
	correlator,genericFormulaFile,
	correlator,modelFile
	correlator,RATETHREHOLD
	correlator,AGRTHREHOLD
	correlator,classRuleN: CLASSIFICATIONRULE, and
	correlator,attributionRuleN

The correlator mode specifies whether SLD will detect anomalies
(normal operation), record observations for training, or both.
The correlator genericScenarioFile, genericFormlaFile, and modelFile specify
the locations of the files containing the scenario rules, the formula rules
and the alert charactorization models.  Defaults are provided, so these would
not normally be specified in the configuration tile.
The RATETHRESHOLD specified a value for alert charactorization scores for
values not specified in the model file.  Defaults for this value are provided
so it would normally not be specified in the configuration file.
The AGETHRESHOLD specified a time duration used to age out old alerts.  Defaults for this value are provided so it would normally not be specified in the configuration file.

Correlator uses files to store models and observations.  These files are
configured through ../common/fileDefs.h and correlatorFileDefs.h  The settings
in the files should be adequate for most users.  (In all description of files
used in this README, the default settings are assumed.)

--------------------------------------TRAINING-------------------------------
Training is the process of having correlator record observations related to
anomalies and then post-processing these observations to incorporate them
into the current models.  Correlator should be run in detect-record mode after
the detectors supplying input have been trained, since correlator training
is intended to charactorize the false positive characteristics of a fully trained
system.

When correlat is in either record or detect-record mode
(see configuration section) correlator will compute the rate scores of each
input alert type, for each server and save those scores in
   ../central/observations/uncollected/HOSTNAME/correlator/STARTTIME/AR.alertsRate
HOSTNAME is the name of the computer on which SLD is running.
STARTTIME is the time at which SLD started.

After a set of observations has been collected (and SLD stopped) the models
are produced by running:
	1) ./collectObs (moves observations from uncollected to collected
		directory (i.e. ../central/observations/collected/HOSTNAME/SLD/STARTTIME)
	2) ./buildArModels.sh (builds a new set of models from
		the set of collected observations)

The new models are stored in ../central/models/correlator, with a copy of the
prior models stored in ../central/models/correlator.TIMESTAMP.

Note that in the event that bad models are created (e.g. an attack accidently
occured during observation collection), the offending TIMESTAMP directory can
be removed and the ./buildSldModels.sh program re-run to build a set of models
with out the offending observations.

---------------------------FORMULA--------------------------------
Formulas are specified in the formula's file ../central/models/formula.
The formula file comprises multiple formulas have the following syntax
	FORMULANAME
		ALERTNAME	THRESHOLD	COEFFICIENT
		ALERTNAME	THRESHOLD	COEFFICIENT
FORMULANAME is an arbitrary name for the formula.  FORMULANAMES must be
unique.
ALERTNAME is the name of an alert (from alert.h).
THRESHOLD is a positive integer or -1.  If THRESHOLD is a positive integer,
then THRESHOLD will be used instead of the THRESHOLD specified in the 
model for that alert.  If THRESHOLD is -1, the THRESNOLD from the model wil
be used.
COEFFIEINT is a postive integer.  It is a multiplier for the threshold.
A formula contains multiple ALERT specification lines.  A formula is true
if each ALERT specification line is true.  An ALERT specification line is
ture if the current rate of alerts of type ALERT exceeds THRESHOLD * COEFFICIENT.
Note if THREHOLD is -1 THRESHOLD is replaced by the value in the model file.

An example formula is:
	F1 {
		SLDTIMELOWMAX -1 1
		PRDZERO -1 1
	}
This specifies that the formula F1 is true if the scores for both SLDTIMELOWMAX
and PRDZERO exceed the values specified for them in the model file.  A formula
becoming true means an attack has been detected.

---------------------------SCENARIOS--------------------------------
SCENARIOS express a time-ordered sequence of alerts.
The syntax of a scenario is:
	SCENARIONAME {
		SEC duration
		ALERT1
		ALERT2
		...
		ALERTN
	}
Duration is the number of seconds over which the scenario is evaluated.
ALERT is the name of an alert.
The scenario is true if withing duration seconds, the input order of alerts
corresponds to ALERT1, followed by ALERT2, ... ending with ALERTN.
If a scenario is true, an attack is detected.
		
---------------------------CLASSIGICATION--------------------------------
Classifications are specified in the common/config file.

Ther syntax of a classification is:
correlator,attributionRuleNumber: RuleName ALERT1(P1) ALERT2(P2) ... ALERTN

attributionRuleNumber is a unique name for rule.
RuleName is a user readable name for the rule.
ALERT1 through ALERTN specify the alerts that must have non zero scores for
the rule to be true.
In this way the attributionRules are similar to the formua rules, however
the ALERTs allow specification of addition parameters the increase the
specificity.
Alerts may include a field (called details) that allow specification of the 
precise reason for the alert. These parameters can be specified in parethesis
after the alert name.  An alert specification is true if the alert with
the specified attributes occurs.
For example:
	correlator,classRule12: AF0 PDDSTARTANOM PRDALLMAX(-Ack)
specifies that the combination of a PDDSTARTANOM alert and a PRDALLMAX alert
for which the incoming ACK ratio was exceeded indicate that an attack to be
reported as AF0 has been detected.  (This example detectrs some TCP ackfloods).
---------------------------ATTRIBUTION--------------------------------
Attribution takes attack detection one step further, to the level of trying to
identify the clientIPs participating in the attack.
Attribution can only be done if the detectors producing alerts can also
indicate the client IP.  (Currently PDD and SLD provide this information, but
PRD does not).
Attribution is based on rules in common/config such as:
	correlator,attributionRule4: SF1 PDDSTARTANOM SLDTIMEZERO
This rule means that if attack SF1 (as defined in a classification rule) occurs,
correlator is to search its history of recent alerts and identify those client IP
for which PDDSTARTANOM and SLDTIMEZERO alerts occurred.  These clients are
reported as the GUILTY clients, that is, those reponsible for the attack.
Note that in many cases these will be spoofed client addresses.
---------------------------RUNNING--------------------------------
correlator is built as a C library.  It is started by invoking the function
correlatorMain() from a user-provided C program.  correlatorMain().
---------------------------LOGGING-------------------------------
A simple API is used for logging:
logMessage(FILE *S,char *F,int L,VNSPINTF STYLE ARGS);
S is the open stread you want the log message to go to (e.g. stderr)
F and L are a string and integer that will appear on the output line
	using __FUNCTION__ for f and __LINE__ for L make sense
	the rest of the args are just like printf.
For example logMessage(stderr,__FUNCTION__,__LINE__,"THIS IS MESSAGE %d",10);
----------------------------------OUTPUT----------------------------
Correlator's output (attack detection) is recorded in the correlator's log
file (/tmp/correlator.err).  The proceedure used to run the entire system
(see parent directory README) extracts relevant information from correlator's
log file and displays it in convenient form for real time dieplay.

Correlator also outputs the file /tmp/classification.csv, which is a time
series of all incoming alerts and their scores.  This file can be very useful
in developing classification rules.

Correlator also outputs /tmp/GUILTY which lists the addresses of the clients
found responsible for attacks.

Note that the operational procedure specified in the parent directory consolidates
these files /tmp files into a user specifed directory.
-----------------------FILE ORGAINIZATION----------------------------
tstCorrelator.c: main program, invokes library based detector
correlator.c: top-level correlator code
configureCorrelator.c: code for configuring correlator
alertRate.c: code for tracking alert scores
networkEvidence.c: code for tracking alert evidence over time
parseLine.cL utility for reading files
classifier.c: code for classifying input alerts into attacks base on
	classification rules
formula.c: code for detecting attacks based on formulas
processAlerts.c: top-level code for handling input alerts
getCorrelatorParameter.c: utility for converting defines in to shell usable values
processAR.c: off-line model builder
getMyIps.c: utility for obtaining my ip address
scenario.c: code for classifying input alerts based on scenarios
correlatorLog.c: logging
loadAR.c: code for inputing formulas
alert.h: defines alerts
baseProg.h: defines structure for recieving incoming alerts
correlatorLog.h: logging
networkEvidence.h: defines structures for recording network evidence from alerts
alertRate.h: alert scoring structures
classifier.h: classifier structures
defs.h: general definitions
scenario.h: scenarior structures
ar.h: formula structures
correlatorFileDefs.h: file locations
formula.h: formula structures

