#!/bin/sh

route add default gw 192.168.0.1 eth0

# echo "1" > /proc/sys/net/ipv4/ip_forward

# iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
# iptables -A FORWARD -i eth0 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT
# iptables -A FORWARD -i wlan0 -o eth0 -j ACCEPT

/opt/initial-ntpdate.sh 2>&1 | /usr/bin/logger -t time-sync &
