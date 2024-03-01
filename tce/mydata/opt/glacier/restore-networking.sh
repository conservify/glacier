#!/bin/sh

set -x

ifconfig -a
ip r
ip a show eth0