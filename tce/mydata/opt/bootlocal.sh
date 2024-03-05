#!/bin/sh

# Start openssh daemon, this will also generate keys and may take some time
# based on available entropy.
/usr/local/etc/init.d/openssh start

# Execute configuration based on our hostname.
/opt/`hostname`/bootlocal.sh

# Bring up wireguard, this happens last because it'll need connectivity and may
# time out trying to resolve the peer if the network is down.
modprobe -a ipv6 wireguard
/usr/local/bin/wg-quick up wg0-`hostname`