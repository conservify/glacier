#!/bin/sh

if false; then
    /app/tunneller-wrapper --syslog tunneller-ssh --remote-port 7003 \
                        --log /var/log/tunneller.log \
                        --key /home/tc/.ssh/id_rsa --server 34.201.197.136 &

    /app/tunneller-wrapper --syslog tunneller-syslog --remote-port 514 \
                        --local-port 1514 --log /var/log/tunneller-rsyslog.log \
                        --key /home/tc/.ssh/id_rsa --server 34.201.197.136 --reverse &
fi