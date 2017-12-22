#!/bin/bash

ip route del 169.254.128.131
ip route del 169.254.128.132
ip route del 169.254.128.1

ifconfig enx00e04c68061c 169.254.128.5

ip route add 169.254.128.131 via 169.254.128.5
ip route add 169.254.128.1   via 169.254.128.5

ifconfig enx00e04c680493 169.254.128.6
ip route add 169.254.128.132 via 169.254.128.6
