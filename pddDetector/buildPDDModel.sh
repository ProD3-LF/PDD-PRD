if test "$PD3_PREFIX" == ""
then
	echo "PD3_PREFIX not set, using parent dir"
else
	PD3_PREFIX="$PD3_PREFIX/"
fi
echo "THIS WILL NOT WORK FOR PUBLIC"
PDDMODELDIR=$PD3_PREFIX`./getPddParameter PDDMODELDIR`
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
