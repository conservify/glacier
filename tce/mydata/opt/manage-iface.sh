#!/bin/sh

IFACE=$1
IP=$2
TAG="manage-$IFACE"
DELAY=2
HOST=`hostname`
BASEDIR=$(dirname "$0")

function log() {
    /usr/bin/logger -t $TAG "$@"
}

log "Configuring iface $IFACE"

while /bin/true; do
    ifconfig $IFACE > /dev/null 2>&1
    if [ "$?" == 0 ]; then
        CURRENT_IP=`ifconfig $IFACE 2>/dev/null|awk '/inet addr:/ {print $2}'|sed 's/addr://'`
        if [ "$CURRENT_IP" = "$IP" ]; then
            log "Currently '$IFACE' is '$CURRENT_IP', leaving alone..."
            DELAY=600
        else
            log "Currently '$IFACE' is '$CURRENT_IP', changing..."
            ifconfig "$@" | /usr/bin/logger -t "$TAG" 2>&1
            UP="/opt/$HOST/iface-$IFACE-up.sh"
            log "Running $UP"
            sh $UP
            DELAY=2
        fi
    else
        log "Iface $IFACE is missing..."
    fi

    sleep $DELAY
done
