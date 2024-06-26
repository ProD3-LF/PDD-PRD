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
if test "$PD3_PREFIX" == ""
then
        echo "PD3_PREFIX not set, using parent dir"
else
        PD3_PREFIX="$PD3_PREFIX/"
fi
OBSDIR=$PD3_PREFIX`./getCorrelatorParameter OBSCOLLECTEDCORRELATOR`
rm -rf /tmp/AROBS
mkdir /tmp/AROBS
nfiles=`ls -d $OBSDIR | wc -l`
echo "Found $nfiles observation files"
echo "Copying observation files to temporary work area"
for i in $OBSDIR
do
	ts=`echo $i | sed -e "s/^.*\///"`
	cp $i/AR.alertsRate /tmp/AROBS/AR.alertsRate.$ts
done
echo "analyzing observation files"
./processAR -i /tmp/AROBS
echo "cleaning up temporary work area"
rm -rf /tmp/AROBS
echo "done"
