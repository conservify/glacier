#!/bin/sh

/app/tunneller --remote-port 7002 --log /var/log/tunneller.log --key /home/tc/.ssh/id_rsa --server 34.201.197.136 &

/app/uploader --url https://code.conservify.org/geophone \
              --pattern "([^\.]+)_(\d{8})_(\d{6}).bin$" --watch \
              --token "zddgXMjr_YI2e87G0mch6tXHMupLGZ6PZ58mHOSdqJtQ566PJj8mzQ" \
              --archive \
		      /app/data &

/app/uploader --url http://code.conservify.org/geophones \
              --pattern "(\d{14}).dig1.ch1.KMI.evt$" --watch \
              --token "zddgXMjr_YI2e87G0mch6tXHMupLGZ6PZ58mHOSdqJtQ566PJj8mzQ" \
		      /app/data/obsidian
