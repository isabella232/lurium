#!/bin/bash 
/sbin/start-stop-daemon --stop --quiet --pidfile /var/run/controlserver

#@reboot /sbin/start-stop-daemon --start --quiet --exec /usr/local/bin/tracd -- --daemonize --pidfile /var/run/tracd --port 8080 /var/trac/code --basic-auth="code,/etc/nginx/.htpasswd,Restricted" --user=www-data --group=www-data
