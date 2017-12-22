#!/bin/sh

echo "1" > /proc/sys/net/ipv4/ip_forward

INTERNET=enp3s0
INTERNAL=enx00e04c68061c

iptables -t nat -A POSTROUTING -o $INTERNET -j MASQUERADE
iptables -A FORWARD -i $INTERNAL -o $INTERNAL -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -A FORWARD -i $INTERNET -o $INTERNET -j ACCEPT
