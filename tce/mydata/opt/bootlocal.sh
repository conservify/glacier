#!/bin/sh

# Start serial terminal
# /usr/sbin/startserialtty &

# Set CPU frequency governor to ondemand (default is performance)
echo ondemand > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

# Load modules
/sbin/modprobe i2c-dev

# Start openssh daemon
/usr/local/etc/init.d/openssh start

# Execute configuration based on our hostname.
NAME=/opt/`hostname`/bootlocal.sh
echo "Executing $NAME" | /usr/bin/logger
sh $NAME | /usr/bin/logger
