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
PDDMODELDIR=`./getPddParameter PDDMODELDIR`
NOW=`date +"%s"`
function execCmd {
        eval $1
        if test "${?}" != "0"
        then
                echo $1 failed
		exit -1
        fi
}
function initModels {
	echo "init models"
	execCmd "rm -rf $PDDMODELDIR"
	execCmd "mkdir -p $PDDMODELDIR"
	for i in $SEQSIZES
	do
		touch -d 01/01/1970 $PDDMODELDIR/TCPMODEL.$i
		touch -d 01/01/1970 $PDDMODELDIR/TCPSTARTMODEL.$i
	done
}
if [ -d $PDDMODELDIR ]
then
	echo "backing up last models to $PDDMODELDIR.$NOW"
	execCmd "mkdir $PDDMODELDIR.$NOW"
	execCmd "cp $PDDMODELDIR/*MODEL.[0-9]* $PDDMODELDIR.$NOW"
else
	echo "creating initial empty $PDDMODELDIR"
	initModels
fi
shopt -s extglob
while getopts a:h:c option
do
        case "${option}"
        in
        a) APPS=`echo $APPS ${OPTARG}`;;
        h) HOSTS=`echo "$HOSTS ${OPTARG}"`;;
	c) initModels;;
        esac
done
echo APPS=$APPS
echo HOSTS=$HOSTS
for h in $HOSTS
do
	execCmd "./pddModel --host $h"
done
printf "%s %-55s %10s %10s %s\n" "$(tput smul)" "MODELNAME" "PRIOR SIZE" "NEW SIZE" "$(tput rmul)"
for fn in $PDDMODELDIR/*MODEL.*
do
	f=`echo $fn | sed -e "s/^.*\///"`
	if [ -e $PDDMODELDIR.$NOW/$f ]
	then
		before=`wc -l $PDDMODELDIR.$NOW/$f`
		before=`echo $before | sed -e "s/ .*$//"`
	else
		before=0
	fi
	after=`wc -l $PDDMODELDIR/$f`
	after=`echo $after | sed -e "s/ .*$//"`
	printf "%-55s %10s %10s\n" "$fn" "$before" "$after"
	#echo "$fn	$before	$after"
done
exit
