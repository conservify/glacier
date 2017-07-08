#!/bin/sh

/app/tunneller-wrapper --syslog tunneller-ssh --remote-port 7002 --log /var/log/tunneller.log --key /home/tc/.ssh/id_rsa --server 34.201.197.136 &

/app/tunneller-wrapper --syslog tunneller-syslog --remote-port 514 --local-port 1514 --log /var/log/tunneller-rsyslog.log --key /home/tc/.ssh/id_rsa --server 34.201.197.136 --reverse &

/app/uploader-wrapper --url https://code.conservify.org/geophone \
                      --syslog uploader-geophone \
                      --log /var/log/uploader-geophone.log \
                      --pattern "([^\.]+)_(\d{8})_(\d{6}).bin$" --watch \
                      --token "zddgXMjr_YI2e87G0mch6tXHMupLGZ6PZ58mHOSdqJtQ566PJj8mzQ" \
                      --archive \
		              /app/data &

/app/uploader-wrapper --url http://code.conservify.org/geophones \
                      --syslog uploader-obsidian \
                      --log /var/log/uploader-obsidian.log \
                      --pattern "(\d{14}).dig1.ch1.KMI.evt$" --watch \
                      --token "zddgXMjr_YI2e87G0mch6tXHMupLGZ6PZ58mHOSdqJtQ566PJj8mzQ" \
		              /app/data/obsidian

/app/adc-wrapper > /var/log/adc.log 2>&1
