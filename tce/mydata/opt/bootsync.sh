#!/bin/sh
# put other system startup commands here, the boot process will wait until they complete.
# Use bootlocal.sh for system startup commands that can run in the background 
# and therefore not slow down the boot process.
/usr/bin/sethostname box

echo "169.254.128.129 lodge" >> /etc/hosts
echo "169.254.128.130 glacier" >> /etc/hosts
echo "nameserver 8.8.8.8" >> /etc/resolv.conf

# Data folders.
mkdir -p /data
ln -sf /mnt/mmcblk0p2/data/geophone /data/geophone
ln -sf /mnt/mmcblk0p2/data/obsidian /data/obsidian

# Execute configuration based on our hostname.
NAME=/opt/`hostname`/bootsync.sh
echo "Executing $NAME" | /usr/bin/logger
sh $NAME | /usr/bin/logger

# Do background stuff now that everything is ready.
/opt/bootlocal.sh &
