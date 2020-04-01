#!/bin/bash

# chkconfig: 2345 99 80
# description: node service controller


UTEST_NAME=dms_vdd_utest
DMSD_NAME=dmsd
DMS_VDD_NAME=dms_vdd

RETVAL=0

####################################
#         check functions          #
####################################
check_something ()
{
        runflag=0
        if [ -s $nscpidfile ]; then
                nscpid=$(cat $nscpidfile)
                if ps -p $nscpid > /dev/null
                then
                        runflag=1
                else
                        runflag=0
                fi
        else
                runflag=0
        fi

        return $runflag
}

check_utest ()
{
    UTEST_RC=`lsmod | grep $UTEST_NAME | awk 'match($1, '\"$UTEST_NAME\"'){print $3}'`

    echo $UTEST_RC
    
    return $UTEST_RC
}

check_dmsd ()
{
    DMSD_PID=`ps aux | grep $DMSD_NAME | grep -v grep | awk '{print $2}'`
    
    echo $DMSD_PID

    return $DMSD_PID
}

check_dms_vdd ()
{
    DMS_VDD_RC=`lsmod | grep $DMS_VDD_NAME | awk 'match($1, '\"$DMS_VDD_NAME\"'){print $3}'`

    echo $DMS_VDD_RC
    
    return $DMS_VDD_RC
}

####################################
#       remove dms_vdd_utest       #
####################################
stop_utest ()
{

    RETVAL=1
    
    #find dms_vdd_utest module first
    #UTEST_RC=`lsmod | grep $UTEST_NAME | awk 'match($1, '$UTEST_NAME'){print $3}'`
    #check_utest
    UTEST_RC=$(check_utest)
    
    echo "UTEST_RC = $UTEST_RC"
    
    if [ "$UTEST_RC" ]; then
        `rmmod $UTEST_NAME`
    
        if [ $? -eq 0 ]; then
            echo -n "$UTEST_NAME stop: "
            echo -e '\033[0;32m OK \033[0;39m'
            RETVAL=0;
        else
            echo "$UTEST_NAME can't be removed!"
            echo -n "$UTEST_NAME stop: "
            echo -e '\033[0;31m FAIL \033[0;39m'
        fi
    else
        echo "$UTEST_NAME not exist!"
        echo -n "$UTEST_NAME stop: "
        echo -e '\033[0;31m FAIL \033[0;39m'
    fi
    
    return $RETVAL
}

####################################
#             kill dmsd            #
####################################
stop_dmsd ()
{
    RETVAL=1
    #DMSD_PID=`ps aux | grep $DMSD_NAME | grep -v grep | awk '{print $2}'`
    
    DMSD_PID=$(check_dmsd)
    echo $DMSD_PID
    
    if [ "$DMSD_PID" ]; then
            echo "$DMSD_NAME is running!  pid: $DMSD_PID, removing it~"
		#kill -2 $DMSD_PID
        kill $DMSD_PID
    
        if [ $? -eq 0 ]; then
            echo "removing $DMSD_NAME OK~ retcode = $?"
            echo -n "$DMSD_NAME stop: "
            echo -e '\033[0;32m OK \033[0;39m'
            RETVAL=0
        else
            echo "removing $DMSD_NAME fail! retcode = $?"
            echo -n "$DMSD_NAME stop: "
            echo -e '\033[0;31m FAIL \033[0;39m'
        fi
    else
            echo "$DMSD_NAME is NOT running!"
            echo -n "$DMSD_NAME stop: "
            echo -e '\033[0;31m FAIL \033[0;39m'
    fi
    
    return $RETVAL
}

####################################
#          remove dms_vdd          #
####################################
stop_dms_vdd ()
{
    RETVAL=1

    #find dms_vdd module first
    #DMS_VDD_RC=`lsmod | grep $DMS_VDD_NAME | awk 'match($1, '$DMS_VDD_NAME'){print $3}'`
    #check_dms_vdd
    DMS_VDD_RC=$(check_dms_vdd)
    
    echo "DMS_VDD_RC = $DMS_VDD_RC"
    
    if [ "$DMS_VDD_RC" ]; then
            `rmmod $DMS_VDD_NAME`
    
            if [ $? -eq 0 ]; then
                    echo "$DMS_VDD_NAME removed!"
                    echo -n "$DMS_VDD_NAME stop: "
                    echo -e '\033[0;32m OK \033[0;39m'
                    RETVAL=0
            else
                    echo "$DMS_VDD_NAME can't be removed!"
                    echo -n "$DMS_VDD_NAME stop: "
                    echo -e '\033[0;31m FAIL \033[0;39m'
            fi
    else
            echo "$DMS_VDD_NAME not exist!"
            echo -n "$DMS_VDD_NAME stop: "
            echo -e '\033[0;31m FAIL \033[0;39m'
    fi
    
    return $RETVAL
}


stop ()
{

    RETVAL=0
    stop_utest
    RETVAL=`expr $RETVAL + $?`
    stop_dmsd
    RETVAL=`expr $RETVAL + $?`
    stop_dms_vdd
    RETVAL=`expr $RETVAL + $?`
    
	if [ $RETVAL -eq 0 ]; then
	    echo "stop procedure done~"
	else
    	echo "stop procedure has $RETVAL fails!"
	fi
    
    return $RETVAL;
}

start ()
{

	echo 'I am not ready!'

}

recompile_and_run ()
{

	make clean
	make utest
	cd bin; `./$UTEST_NAME -t`
}

create_volumes ()
{
	echo "create volume by size: $1 MB"
	cd bin; `./$UTEST_NAME -c $1`

}

####################################
#           switch cases           #
####################################
case "$1" in
        start)
                start
                ;;
        stop)
                stop
                ;;
        restart)
                stop
                start
                ;;
        status)
                status_at
                ;;
	cr)
		recompile_and_run
		;;
	cv)
		create_volumes $2
		;;
        *)
                echo $"Usage: $0 {start|stop|restart|status}"
                exit 1

esac
exit $RETVAL

