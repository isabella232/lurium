#!/bin/bash 

GAMESERVER=$1

if [ -z "$GAMESERVER" ];
then
GAMESERVER="default0"
fi

PIDFILE=/var/run/nibbles_${GAMESERVER}

/sbin/start-stop-daemon --stop --quiet --pidfile $PIDFILE

