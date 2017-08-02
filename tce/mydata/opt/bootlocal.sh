#!/bin/sh

# Set CPU frequency governor to ondemand (default is performance)
echo powersave > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

# Load modules
/sbin/modprobe i2c-dev

# Start openssh daemon
/usr/local/etc/init.d/openssh start

# Start hamachi daemon.
/usr/local/bin/hamachid

# Execute configuration based on our hostname.
NAME=/opt/`hostname`/bootlocal.sh
echo "Executing $NAME" | /usr/bin/logger
sh $NAME | /usr/bin/logger

# Sync time
ntpdate tick.ucla.edu
ntpd
