#!/bin/sh
# put other system startup commands here, the boot process will wait until they complete.
# Use bootlocal.sh for system startup commands that can run in the background 
# and therefore not slow down the boot process.
/usr/bin/sethostname box

/opt/bootlocal.sh &

echo "169.254.128.129 lodge" >> /etc/hosts
echo "169.254.128.130 glacier" >> /etc/hosts

# Execute configuration based on our hostname.
NAME=/opt/`hostname`/bootsync.sh
echo "Executing $NAME" | /usr/bin/logger
sh $NAME | /usr/bin/logger

mkdir -p /data
mount /dev/sda1 /data
