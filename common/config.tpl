
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
pddmode: detect-record
#pddservers: 127.0.0.1:5964
pddservers: SERVER:443
prdmode: detect-record
#prdservers: 127.0.0.1:5964
prdservers: SERVER:443
sldservers: SERVER:443
sldmode: detect-record
correlator,mode: detect-record
correlator,classRule1: AF1 PRDMAX(-Ack,+Ack) PRDMAX(+Rst,-Syn)
correlator,classRule2: getFlood PRDALLMAX(+Ack) PRDMAX(+Ack,-Ack) PRDMAX(+Ack,-Syn) PRDMAX(+Ack,+AckSyn)
correlator,classRule3: stomp PRDMAX(-AckPsh,-Syn) PRDMAX(-AckPsh,+AckSyn) PRDMAX(-AckPsh,+AckFin) PRDMAX(+Rst,-Syn)
correlator,classRule4: SF1 PRDALLMAX(+AckSyn) PRDMAX(+AckSyn,-AckFin) PRDMAX(+AckSyn,-AckPsh) PRDMAX(+AckSyn,-Ack)
correlator,classRule5: SF1SPF SLDTIMEZERO PRDALLMAX(+AckSyn) PRDMAX(+AckSyn,-AckFin) PRDMAX(+AckSyn,-AckPsh) PRDMAX(+AckSyn,-Ack)
correlator,classRule6: SF2 PDDSTARTANOM PRDALLMAX(-Syn) PRDALLMAX(-Ack) PRDALLMAX(+AckPsh) PRDMAX(-Syn,-AckFin) PRDMAX(+Rst,-Syn) PRDMAX(+AckSyn,-Syn)
correlator,classRule7: SF2SPF SLDTIMEZERO PDDSTARTANOM PRDALLMAX(-Syn) PRDALLMAX(-Ack) PRDALLMAX(+AckPsh) PRDMAX(-Syn,-AckFin) PRDMAX(+Rst,-Syn) PRDMAX(+AckSyn,-Syn)
correlator,classRule8: AF2 PDDSTARTANOM PRDALLMAX(-Ack) PRDMAX(-Ack,+AckPsh) PRDSD2(+Rst,-AckFin) PRDSD2(+Rst,-Syn) PRDSD2(+Rst,+AckSyn)
correlator,classRule9: AF3 PDDSTARTANOM PRDALLMAX(-Ack) PRDALLMAX(+AckPsh) PRDMAX(-Ack,+AckPsh) PRDMAX(+Rst,-AckFin)
correlator,classRule10: SF3 PDDSTARTANOM PRDALLMAX(-Syn) PRDMAX(-Syn,-AckFin) PRDMAX(-Syn,+AckSyn) PRDMAX(+Rst,-Syn) PRDMAX(+AckSyn,-Syn) PRDMAX(-Syn,-AckFin)
correlator,classRule11: SF3SPF SLDTIMEZERO PDDSTARTANOM PRDALLMAX(-Syn) PRDMAX(-Syn,-AckFin) PRDMAX(-Syn,+AckSyn) PRDMAX(+Rst,-Syn) PRDMAX(+AckSyn,-Syn) PRDMAX(-Syn,-AckFin)
correlator,classRule12: AF0 PDDSTARTANOM PRDALLMAX(-Ack)
correlator,classRule13: ST0 PDDANOM PDDSTARTANOM PRDALLMAX(-Ack) PRDALLMAX(+AckPsh) PRDALLMAX(-AckPsh) PRDALLMAX(+Rst) PRDMAX(+Rst,-AckFin)
correlator,classRule14: AH0 PDDANOM PDDSTARTANOM PRDSD2(-AckFin,+AckSyn) PRDSD2(-AckFin,+Rst) PRDSD2(-AckFin,-Syn) PRDSD2(+AckSyn,-AckFin) PRDSD2(+Rst,-AckFin) PRDSD2(-Syn,-AckFin)
correlator,classRule15: AH1 PDDANOM PDDSTARTANOM PRDMAX(-AckFin,+AckSyn) PRDMAX(-AckFin,+Rst) PRDMAX(-AckFin,-Syn) PRDMAX(+AckSyn,-AckFin) PRDMAX(+Rst,-AckFin) PRDMAX(-Syn,-AckFin)
correlator,attributionRule1: AF1 PDDSTARTANOM
correlator,attributionRule2: getFlood PDDSTARTANOM
correlator,attributionRule3: stomp PDDSTARTANOM
correlator,attributionRule4: SF1 PDDSTARTANOM
correlator,attributionRule5: SF2 PDDSTARTANOM
correlator,attributionRule6: AF2 PDDSTARTANOM
correlator,attributionRule7: AF3 PDDSTARTANOM
correlator,attributionRule8: SF3 PDDSTARTANOM
correlator,attributionRule9: AF0 PDDSTARTANOM
correlator,attributionRule10: AH0 PDDSTARTANOM
correlator,attributionRule11: AH1 PDDSTARTANOM
