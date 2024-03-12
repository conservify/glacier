#!/bin/sh

/opt/manage-iface.sh eth0 192.168.0.50 netmask 255.255.255.0 broadcast 192.168.0.255 up &

if [ -d /usb ]; then
    DATA=/usb/data
else
    DATA=/home/tc/card/data
fi

mkdir -p $DATA
mkdir -p $DATA/config/syncthing
mkdir -p /home/tc/.local/state
ln -sf $DATA/config/syncthing /home/tc/.local/state/syncthing
chown -R tc: /home/tc/.local
chown -R tc: $DATA

# This will not overwrite if a key and identity is already there.
/opt/syncthing/syncthing generate

su tc -c "/opt/syncthing/syncthing" 2>&1 | /usr/bin/logger -t syncthing &

su tc -c "/home/tc/geophone-wrapper.sh --path $DATA/geophone --done /home/tc/file-done.sh" &

# eof