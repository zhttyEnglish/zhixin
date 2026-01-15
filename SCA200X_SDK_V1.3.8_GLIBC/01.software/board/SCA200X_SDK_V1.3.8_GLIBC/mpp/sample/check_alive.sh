#!/bin/sh

TRY_CNT=0
COUNT=10
TIME=3

check_alive()
{
	PID=`ps -aux | grep $1 | grep -v grep | wc -l`
#	echo "PID = $PID"
	if [ $PID -eq 0 ];then
		echo "?????? auxiliary_control_app die , restart ??????"
		/userdata/auxiliary_control_app &

		TRY_CNT=$((TRY_CNT+1))
#		echo "TRY_CNT=TRY_CNT+1"

		if [ $TRY_CNT -gt $COUNT ];then
			sudo reboot
#		else
#			echo "TRY_CNT = $TRY_CNT"
		fi

	elif [ $PID -eq 1 ];then
#		echo "!!!!!! auxiliary_control_app is alive !!!!!!"
		TRY_CNT=0
	fi
}

while true
do
	check_alive auxiliary_control_app
#	echo "check_alive auxiliary_control_app"
	sleep $TIME
done

