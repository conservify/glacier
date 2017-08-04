#!/bin/sh

# Start watchdog
# /usr/sbin/watchdog -T 15 -t 5 /dev/watchdog

# Set CPU frequency governor to ondemand (default is performance)
echo powersave > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

# Start openssh daemon
/usr/local/etc/init.d/openssh start

# Start hamachi daemon.
/usr/local/bin/hamachid

# Execute configuration based on our hostname.
/opt/`hostname`/bootlocal.sh

# Wait for the wireless before we do some things.
/opt/await-wireless.sh
