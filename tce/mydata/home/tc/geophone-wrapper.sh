#!/bin/bash

while /bin/true; done
    /home/tc/geophone.py "$@" 2>&1 | /usr/bin/logger -t geophone

    sleep 1

    /etc/periodic/5min/delete-old | /usr/bin/logger -t geophone-restarting

    sleep 1
done
