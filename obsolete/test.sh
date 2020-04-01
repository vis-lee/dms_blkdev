#!/bin/bash

####################################
#       remove dms_vdd_utest       #
####################################
#find dms_vdd_utest module first
UTEST_NAME=dms_vdd_utest

UTEST_RC=`lsmod | grep $UTEST_NAME | awk 'match($1, '\"$UTEST_NAME\"'){print $3}'`

echo "UTEST_RC = $UTEST_RC"


