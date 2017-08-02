#!/bin/sh

ifconfig eth1 169.254.128.129 netmask 255.255.255.0 broadcast 169.254.128.255 up

/etc/init.d/dhcp.sh

echo "1" > /proc/sys/net/ipv4/ip_forward

iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
iptables -A FORWARD -i eth1 -o eth1 -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -A FORWARD -i eth0 -o eth0 -j ACCEPT
