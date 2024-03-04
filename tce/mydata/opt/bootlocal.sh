#!/bin/sh

# Start openssh daemon
/usr/local/etc/init.d/openssh start

# Execute configuration based on our hostname.
/opt/`hostname`/bootlocal.sh

# Wait for the wireless before we do some things.
# /opt/await-wireless.sh
