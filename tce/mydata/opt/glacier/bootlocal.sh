#!/bin/sh

# Start gpsd if we have a serial device.
if [ -e /dev/ttyACM0 ]; then
    gpsd /dev/ttyACM0 2>&1 | /usr/bin/logger -t gpsd &
fi