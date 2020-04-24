#!/bin/bash 
/sbin/start-stop-daemon --start --quiet -b -m --pidfile /var/run/controlserver --exec /home/calvin/src/nibbles/control/control -- does not parse arguments

#@reboot /sbin/start-stop-daemon --start --quiet --exec /usr/local/bin/tracd -- --daemonize --pidfile /var/run/tracd --port 8080 /var/trac/code --basic-auth="code,/etc/nginx/.htpasswd,Restricted" --user=www-data --group=www-data
