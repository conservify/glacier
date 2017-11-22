#!/bin/bash

HOST=ham-lodge
UAH=tc@$HOST

set -xe

ssh $UAH "cp /mnt/mmcblk0p2/tce/mydata.tgz ~/lodge-mydata.tgz"
ssh $UAH "scp glacier:/mnt/mmcblk0p2/tce/mydata.tgz glacier-mydata.tgz"

scp "$UAH:*-mydata.tgz" .
