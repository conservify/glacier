#!/bin/sh

chown -R tc. /home/tc/card/data

su tc -c "/home/tc/geophone.py --path /home/tc/card/data/geophone" 2>&1 | /usr/bin/logger -t geophone &

su tc -c "/opt/syncthing/syncthing" 2>&1 | /usr/bin/logger -t syncthing &