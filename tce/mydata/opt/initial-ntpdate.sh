#!/bin/sh

NTP_SERVER=us.pool.ntp.org

YEAR=`date "+%Y"`
if [ "$YEAR" = "1970" ]; then
    echo "Bad year, trying to fix time..."
    while /bin/true; do
        ping -c 1 8.8.8.8
        if [ "$?" == 0 ]; then
            ping -c 1 $NTP_SERVER
            if [ "$?" == 0 ]; then
                ntpdate $NTP_SERVER
                if [ "$?" == 0 ]; then
                    exit 0
                fi
            fi
        fi

        sleep 5
    done
else
    echo "Year looks good, skipping ntpdate"
fi
