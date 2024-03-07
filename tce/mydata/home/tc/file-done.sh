#!/bin/bash

export $(grep -v '^#' /etc/secure.env | xargs)

FILE=$1
UPLOAD_URL=http://192.168.0.100:8000
UPLOAD_TOKEN=fake-token

set -xe

NAME=`basename $FILE`

echo "$FILE done" | /usr/bin/logger -t "file-done"

rm -rf /tmp/*.wav

time sox $FILE -r 500 /tmp/$NAME | /usr/bin/logger -t "file-done"

/home/tc/uploader --url $UPLOAD_URL \
	--token $UPLOAD_TOKEN \
	--file /tmp/$NAME | /usr/bin/logger -t "file-done"

# eof
