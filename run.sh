#!/bin/bash

# Vars holding bin names
ACCOUNTING="./Accounting"
MISSIONCONTROL="./MissionControl"

while :
do
	if ps ax | grep -v grep | grep $ACCOUNTING > /dev/null
	then
		echo Accounting Running
	else
		$ACCOUNTING &
	fi
	if ps ax | grep -v grep | grep $MISSIONCONTROL > /dev/null
	then
		echo Mission Control Running
	else
		$MISSIONCONTROL &
	fi

	sleep 5
	
done
