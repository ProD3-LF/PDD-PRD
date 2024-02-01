#!/bin/bash
if test "$PD3_PREFIX" == ""
then
	echo "PD3_PREFIX not set, using parent dir"
else
	PD3_PREFIX="$PD3_PREFIX/"
fi
PRDMODELS=$PD3_PREFIX`./getPrdParameter STOREDMODELDIR`
MODELDIR=$PD3_PREFIX`./getPrdParameter MODELDIR`
TMPWORKINGDIR=`./getPrdParameter TMPWORKINGDIR`
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
        h) HOSTS=`echo "$HOSTS|${OPTARG}"`;;
        esac
done
VM=`echo "?($HOSTS)"`
echo $VM
#echo "DID YOU COLLECT?"
#read
execCmd "mkdir -p $PRDMODELS"
execCmd "mkdir $MODELDIR/PRD.$NOW"
execCmd "mv $PRDMODELS/corcof $MODELDIR/PRD.$NOW"
execCmd "mv $PRDMODELS/PRDSTDMODEL $MODELDIR/PRD.$NOW"
execCmd "mv $PRDMODELS/ZPROB $MODELDIR/PRD.$NOW"
rm -rf $TMPWORKINGDIR
#execCmd "cp -p $PRDMODELS/cwmap $MODELDIR/PRD.$NOW"
execCmd "./cpPRDCentralObs $VM"
echo "building Std model"
execCmd "./buildStdModel"
echo "building correlation coefficient  model"
execCmd "./buildCorModel"
echo "building zprob model"
execCmd "./buildZeroModel"
rm -rf $TMPWORKINGDIR
