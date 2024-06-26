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
#!/bin/bash
if test "$PD3_PREFIX" == ""
then
	echo "PD3_PREFIX not set, using parent dir"
else
	PD3_PREFIX="$PD3_PREFIX/"
fi
MODELDIR=$PD3_PREFIX`./getSldParameter MODELDIR`
SLDMODELS=$PD3_PREFIX`./getSldParameter SLDMODELS`
SLDMODELDIR=$PD3_PREFIX`./getSldParameter SLDMODELDIR`
function execCmd {
        eval $1
        if test "${?}" != "0"
        then
                echo $1 failed
        fi
}
NOW=`date +"%s"`
shopt -s extglob
while getopts h: option
do
        case "${option}"
        in
        h) HOSTS=`echo "$HOSTS ${OPTARG}"`;;
        esac
done
#echo "DID YOU COLLECT?"
#read
execCmd "mkdir -p $SLDMODELDIR"
execCmd "mv $SLDMODELDIR $MODELDIR/SLD.$NOW"
for h in $HOSTS
do
execCmd "./buildDensityModel --host $h"
done
