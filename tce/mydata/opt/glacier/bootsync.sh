#!/bin/sh

/opt/manage-iface.sh eth0 169.254.128.130 netmask 255.255.255.0 broadcast 169.254.128.255 up &

/opt/manage-iface.sh eth1 169.254.127.130 netmask 255.255.255.0 broadcast 169.254.128.255 up &
