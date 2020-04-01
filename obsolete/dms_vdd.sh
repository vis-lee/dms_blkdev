#!/bin/bash

####################################
#       remove dms_vdd_utest       #
####################################
#find dms_vdd_utest module first
UTEST_NAME=dms_vdd_utest

UTEST_RC=`lsmod | grep $UTEST_NAME | awk 'match($1, '$UTEST_NAME'){print $3}'`

echo "UTEST_RC = $UTEST_RC"

if [ "$UTEST_RC" ]
then
	`rmmod $UTEST_NAME`

	if [ $? -eq 0 ]
	then
		echo "dms_vdd_utest removed!"
	else
		echo "dms_vdd_utest can't be removed!"
	fi
else
	echo "$UTEST_NAME not exist!"
fi


####################################
#             kill dmsd            #
####################################
DMSD_NAME=dmsd
DMSD_PID=`ps aux | grep $DMSD_NAME | grep -v grep | awk '{print $2}'`

if [ "$DMSD_PID" ]
then
        echo "$DMSD_NAME is running!  pid: $DMSD_PID, removing it~"
	kill -2 $DMSD_PID

	if [ $? -eq 0 ]
	then
		echo "removing $DMSD_NAME OK~ retcode = $?"
	else
		echo "removing $DMSD_NAME fail! retcode = $?"
	fi
else
        echo "$DMSD_NAME is NOT running!"
fi


####################################
#          remove dms_vdd          #
####################################

#find dms_vdd module first
DMS_VDD_NAME=dms_vdd

DMS_VDD_RC=`lsmod | grep $DMS_VDD_NAME | awk 'match($1, '$DMS_VDD_NAME'){print $3}'`

echo "DMS_VDD_RC = $DMS_VDD_RC"

if [ "$DMS_VDD_RC" ]
then
        `rmmod $DMS_VDD_NAME`

        if [ $? -eq 0 ]
        then
                echo "$DMS_VDD_NAME removed!"
        else
                echo "$DMS_VDD_NAME can't be removed!"
        fi
else
        echo "$DMS_VDD_NAME not exist!"
fi


