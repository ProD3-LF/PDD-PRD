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
This directory contains an example tcpdump based sensor for use
with PRD ad PDD.

This sensor is for example purposes only and is of prototype quality.

The sensor (tcpdumpSensor) takes two optional arguments, --pcapFile 
and --speedFactor.
pcapFile's value is the FQN of a pcap file.  If --pcapFile argument is
present tcpdumpSensor uses this file as input to tcpdump.

If the argument is not present tcpdumpSensor runs tcpdump online, using
the command in the file tcpdumpCommand.

The tcpdump command is executed from within tcpdumpSensor as a system command.
In on-line mode, the user should check that the arguments to tcpdump in
the tcpdumpCommand file, (particularily the interface argument)
are suitable for their environment.  The example tcpdump command in
tcpdumpCommand is for monitoring an http server at 192.168.2.1:80, which coincides
with the pcap samples provided in this distribution.  For illustrative purpose there
is a commented out command for monitoring a local vncserver at port 5964.

The server and port specified in the tcpdumpcommand should agree with
the protected server specified in ../common/config, otherwise the 
sensor will not capture packets for the desired protected server.
In the distribution ../common/config is setup for 192.168.2.1:80, so
replay of the samples should work with no adjuestment.


