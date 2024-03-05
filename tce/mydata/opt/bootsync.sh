#!/bin/sh
# put other system startup commands here, the boot process will wait until they complete.
# Use bootlocal.sh for system startup commands that can run in the background 
# and therefore not slow down the boot process.
/usr/bin/sethostname box

export $(grep -v '^#' /etc/secure.env | xargs)

# Set CPU frequency governor to ondemand (default is performance)
echo powersave > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo "169.254.128.129 lodge" >> /etc/hosts
echo "169.254.128.130 glacier" >> /etc/hosts
echo "nameserver 1.1.1.1" >> /etc/resolv.conf

chmod 700 /root

# Fix rsyslog configuration and restart the service.
FILE=/etc/rsyslog.conf.`hostname`
if [ -f $FILE ]; then
    mv $FILE /etc/rsyslog.conf
fi

# Restart services to reload configuration.
pkill rsyslogd && rsyslogd
pkill crond && crond

# Log environment, debugging.
env | /usr/bin/logger -t env

# Mount USB drive if we find one.
if [ -e /dev/sda1 ]; then
    mkdir /usb
    mount /dev/sda1 /usb
fi

# Execute configuration based on our hostname.
/opt/`hostname`/bootsync.sh

# Do background stuff now that everything is ready.
/opt/bootlocal.sh &

# eof