#!/bin/sh
# -*- mode: sh -*-

/usr/bin/logger -t await-wireless "Waiting for wireless link..."

while /bin/true; do
    ping -c 1 glacier
    if [ "$?" == 0 ]; then
        ping -c 1 lodge
        if [ "$?" == 0 ]; then
            /usr/bin/logger -t await-wireless "Wireless up"

            # Fix the keys situation.
            ssh-keyscan lodge > /root/.ssh/known_hosts
            ssh-keyscan glacier >> /root/.ssh/known_hosts
            cp /root/.ssh/known_hosts ~tc/.ssh
            chown tc. ~tc/.ssh/known_hosts

            exit 0
        fi
    fi

    sleep 5
done
