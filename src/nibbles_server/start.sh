#!/bin/bash 

GAMESERVER=$1

if [ -z "$GAMESERVER" ];
then
GAMESERVER="default0"
fi

PIDFILE=/var/run/nibbles_${GAMESERVER}
CONFIGFILE=/home/calvin/src/nibbles/nibbles/${GAMESERVER}.ini

DAEMON=/home/calvin/src/nibbles/nibbles/a.out

echo $PIDFILE
echo $CONFIGFILE

/sbin/start-stop-daemon --start --quiet --background --make-pidfile --pidfile $PIDFILE --startas /bin/bash -- -c "exec $DAEMON config=$CONFIGFILE"

# > /var/log/some.log 2>&1
