#!/bin/sh

# Based on udhcpc script edited by Tim Riker <Tim@Rikers.org>
# Change: Adding call to ntpdate

[ -z "$1" ] && echo "Error: should be called from udhcpc" && exit 1

/usr/bin/logger "udhcpc action called: $@"

RESOLV_CONF="/etc/resolv.conf"
[ -n "$broadcast" ] && BROADCAST="broadcast $broadcast"
[ -n "$subnet" ] && NETMASK="netmask $subnet"

case "$1" in
        leasefail)
                mkdir -p /tmp/udhcpd-failures
                touch /tmp/udhcpd-failures/failure.$$
                NUM=`ls /tmp/udhcpd-failures/failure.* | wc -l`
                /usr/bin/logger "leasefail #$NUM"
                if [ "$NUM" -eq "10" ]; then
                        rm -rf /tmp/udhcpd-failures
                        /usr/bin/logger "leasefail REBOOT"
                        /sbin/reboot
                fi
                ;;

        deconfig)
                /sbin/ifconfig $interface 0.0.0.0
                ;;

        renew|bound)
                /sbin/ifconfig $interface $ip $BROADCAST $NETMASK

                if [ -n "$router" ] ; then
                        echo "deleting routers"
                        while route del default gw 0.0.0.0 dev $interface ; do
                                :
                        done

                        metric=0
                        for i in $router ; do
                                route add default gw $i dev $interface metric $((metric++))
                        done
                fi

                echo -n > $RESOLV_CONF
                [ -n "$domain" ] && echo search $domain >> $RESOLV_CONF
                for i in $dns ; do
                        echo adding dns $i
                        echo nameserver $i >> $RESOLV_CONF
                done

                /opt/initial-ntpdate.sh &

                ;;
esac

exit 0
