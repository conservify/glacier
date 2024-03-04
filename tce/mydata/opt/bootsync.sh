#!/bin/sh
# put other system startup commands here, the boot process will wait until they complete.
# Use bootlocal.sh for system startup commands that can run in the background 
# and therefore not slow down the boot process.
/usr/bin/sethostname box

chmod 700 /root

echo "169.254.128.129 lodge" >> /etc/hosts
echo "169.254.128.130 glacier" >> /etc/hosts
echo "nameserver 1.1.1.1" >> /etc/resolv.conf

# Execute configuration based on our hostname.
/opt/`hostname`/bootsync.sh

# Start time sync
# ntpd

# Do background stuff now that everything is ready.
/opt/bootlocal.sh &

# eof