#!/bin/sh

set -x

for i in 1 2 3; do
    ifconfig -a
    ip r
    ip a show eth0
    date
    pkill udhcpc
    sleep 1
    ifconfig eth0 0.0.0.0
    ifconfig eth0 down
    sleep 2
    ifconfig eth0 up
    sleep 1
    /sbin/udhcpc -S -i eth0 -n -x hostname:lodge -p /var/run/udhcpc.eth0.pid -s /opt/lodge/udhcpc-action.sh 2>&1

    ping -c 1 8.8.8.8
    if [ "$?" == 0 ]; then
        exit 0
    fi
done
