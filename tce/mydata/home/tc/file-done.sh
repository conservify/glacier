#!/bin/bash

FILE=$1

set -xe

echo "$FILE done" | /usr/bin/logger -t "file-done"

time sox $FILE -r 500 /tmp/500hz.wav | /usr/bin/logger -t "file-done"

# eof