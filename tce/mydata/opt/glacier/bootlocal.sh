#!/bin/sh

if false; then
    /app/tunneller-wrapper --syslog tunneller-ssh --remote-port 7002 --log /var/log/tunneller.log --key /home/tc/.ssh/id_rsa --server 34.201.197.136 &

    /app/uploader-wrapper --url https://calgary.ocr.nyc/send \
                        --syslog uploader-geophone \
                        --log /var/log/uploader-geophone.log \
                        --pattern "([^\.]+)_(\d{8})_(\d{6}).bin$" --watch \
                        --token "zddgXMjr_YI2e87G0mch6tXHMupLGZ6PZ58mHOSdqJtQ566PJj8mzQ" \
                        --archive \
                        /data/geophone &

    /app/adc-wrapper &
fi