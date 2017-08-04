#!/bin/sh

YEAR=`date "+%Y"`
if [ "$YEAR" = "1970" ]; then
    /usr/bin/logger "Bad year, trying to fix time..."
    while /bin/true; do
        ping -c 1 8.8.8.8
        if [ "$?" == 0 ]; then
            ping -c 1 tick.ucla.edu
            if [ "$?" == 0 ]; then
                ntpdate tick.ucla.edu
                if [ "$?" == 0 ]; then
                    exit 0
                fi
            fi
        fi

        sleep 5
    done
else
    /usr/bin/logger "Year looks good, skipping ntpdate"
fi
