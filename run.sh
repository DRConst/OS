!/bin/sh

# Vars holding bin names
ACCOUNTING="Accounting"
MISSIONCONTROL = "MissionControl"

# Starting Servers as Daemons( No Terminal )
$ACCOUNTING &
$MISSIONCONTROL &

while :
do

	accRunning = 'pgrep ${ACCOUNTING }'
	msRunning = 'pgrep ${MISSIONCONTROL}'
	
	if [ "${accRunning:-null}" = null ]; then
		$ACCOUNTING &
	fi
	
	if [ "${msRunning:-null}" = null ]; then
		$MISSIONCONTROL &
	fi
		
	sleep 5
	
done