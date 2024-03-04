#!/bin/sh

/opt/manage-iface.sh eth0 192.168.0.50 netmask 255.255.255.0 broadcast 192.168.0.255 up &

chown -R tc. /home/tc/card/data

su tc -c "/home/tc/geophone.py --path /home/tc/card/data/geophone" 2>&1 | /usr/bin/logger -t geophone &

# This will not overwrite if a key and identity is already there.
/opt/syncthing/syncthing generate

su tc -c "/opt/syncthing/syncthing" 2>&1 | /usr/bin/logger -t syncthing &

# eof