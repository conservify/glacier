#!/bin/sh

# Set CPU frequency governor to ondemand (default is performance)
echo powersave > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

# Start openssh daemon
/usr/local/etc/init.d/openssh start

# Fix rsyslog configuration and restart the service.
FILE=/etc/rsyslog.conf.`hostname`
if [ -f $FILE ]; then
    mv $FILE /etc/rsyslog.conf
fi

# Restart services to reload configuration.
pkill rsyslogd && rsyslogd
pkill crond && crond

# Start hamachi daemon.
/usr/local/bin/hamachid

# Execute configuration based on our hostname.
/opt/`hostname`/bootlocal.sh

# Wait for the wireless before we do some things.
/opt/await-wireless.sh
