#!/bin/sh

/opt/manage-iface.sh eth1 169.254.128.129 netmask 255.255.255.0 broadcast 169.254.128.255 up &

/sbin/udhcpc -S -b -i eth0 -x hostname:lodge -p /var/run/udhcpc.eth0.pid -s /opt/lodge/udhcpc-action.sh >/dev/null 2>&1 &
