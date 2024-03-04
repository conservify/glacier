#!/bin/sh
# put other system startup commands here, the boot process will wait until they complete.
# Use bootlocal.sh for system startup commands that can run in the background 
# and therefore not slow down the boot process.
/usr/bin/sethostname box

# Set CPU frequency governor to ondemand (default is performance)
echo powersave > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

chmod 700 /root

echo "169.254.128.129 lodge" >> /etc/hosts
echo "169.254.128.130 glacier" >> /etc/hosts
echo "nameserver 1.1.1.1" >> /etc/resolv.conf

# Fix rsyslog configuration and restart the service.
FILE=/etc/rsyslog.conf.`hostname`
if [ -f $FILE ]; then
    mv $FILE /etc/rsyslog.conf
fi

# Restart services to reload configuration.
pkill rsyslogd && rsyslogd
pkill crond && crond

# Execute configuration based on our hostname.
/opt/`hostname`/bootsync.sh

# Start gpsd if we have a serial device.
if [ -e /dev/ttyACM0 ]; then
    gpsd /dev/ttyACM0 2>&1 | /usr/bin/logger -t gpsd &
fi

# Start time sync
# ntpd

# Do background stuff now that everything is ready.
/opt/bootlocal.sh &

# eof