#!/bin/sh

ifconfig eth0 169.254.128.130 netmask 255.255.255.0 broadcast 169.254.128.255 up
route add default gw 169.254.128.129 eth0

ifconfig eth1 169.254.127.130 netmask 255.255.255.0 broadcast 169.254.128.255 up

echo "1" > /proc/sys/net/ipv4/ip_forward

iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
iptables -A FORWARD -i eth0 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -A FORWARD -i wlan0 -o eth0 -j ACCEPT
