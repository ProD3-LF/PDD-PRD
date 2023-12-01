This directory is the parent directory for
	Protocol Deviant Detector (PDD) ./pddDetector & ./common
	Protocol Ratio Detector (PRD) ./prdDetector & ./common
	tcpdumpSensor ./sensor * ./common
	sample pcap files for input to tcpdumpSensor ./pcapSamples
	models used by PDD and PRD ./central/models/[PDD|PRD]

pddDetector, prdDetector and sensor have READMEs describing each detector.


pddDetector and prdDetector can be run with default settings and previously
collected pcaps for installation testing, however the user will want to produce
their own models.

proceedure:
	0) git clone https://github.com/tenox7/ttyplot.git
		Do this once to get code used for graphing purposes
	1) make - to build everything
	2) ./run pcapSamples/FILE.pcap (where FILE is one of the files in
		pcapSamples
		There are two files in pcapSamples: benign.pcap and synFloodSmall.pcap
		benign.pcap contains a sample of benign traffic
		synFloodSmall.pcap contains a sample of benign traffic with a 
			sample of a synFlood attack overlaid.

note: run assumes an xterm environment.

run will popup xterms for tcpdumpSensor, PDD, and PRD which
give a summary of operation focusing on inputs received and alerts produced.
run will also popup two addition xterms for PDD and PRD showing alert details
and graphing alerts over time.

Training:
PRD and PDD are anomaly detectors and need models of normal behavior from
which to detect anomalies.  the PRD and PDD READMEs describe the training
process.  For testing purposes, this distribution includes models that
where built local using a simulated http server (192.168.2.1:80).
These models may or may not be adequate for any particular environment.
This distribution also includes pcap samples for simulated benign and synflood
traffic to/from the simulated server.  THE MODELS ARE SIMPLY PLACEHOLDERS
FOR TESTING PURPOSES, moreover, the observations associated with them
are not included, and hence the user cannot rebuild these exact models.
It is recommended that after initial testing the user delete the current
models (i.e. rm -rf central/models/[PDD|PRD] and build their own.
Configuration:
The supplied configuration file (common/config) lists the protected server
for both PDD and PRD as the simulated server (192.168.2.1:80) used to
produce the models and the pcaps.  This will allow initial installation
testing.  After install testing the user should adjust the server listed in
common/config to be the IP:PORT of the server they wish to protect.
PCAP files:
the tcpdump command for producing pcap files is basically:
	tcpdump -w PCAPFILENAME -nn -i INTERFACE -tt -s 100 tcp host SERVERIP and port SERVERPORT
where PCAPFILENAME is the name of the file to store the pcap,
INTERFACE is the name of the ethernet interface over which traffic to
and from the server flows and SERVERIP and SERVERPORT are the IP address 
and port of the server.  Other tcpdump options may be substituted at the
user's descretion, however care should be taken to minimize the size of
the pcap file, as large pcaps play back slowly. Generated pcap files
should be placed in the pcapSamples directory.
Diagnostics:
run uses files in /tmp as sinks for stdin and stdout.  These files have
obvious names (e.g. /tmp/prdDetector.err).  These files give basic operational
information about the sensor, PDD, and PRD, and can be consulted to help
resolve problems.  
After cloning some file may not have the right permissions for execution.  Make sure
the following files are executable
	run
	pddDetector/startPddDetector
	pddDetector/firstTime
	pddDetector/buildPDDModel.sh
	pddDetector/collectObs
	prdDetector/startPrdDetector
	prdDetector/firstTime
	prdDetector/buildPRDModels.sh
	prdDetector/cpPRDCentralObs
	sensor/tcpdumpSensor
	sensor/tcpdumpCommand
