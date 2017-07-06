#!/bin/bash

sudo umount card; /bin/true
sudo losetup -d /dev/loop0; /bin/true

set -xe 

cp piCore-9.0.3.img piCore-9.0.3-larger.img

fdisk -l piCore-9.0.3-larger.img

dd if=/dev/zero bs=1M count=100 >> piCore-9.0.3-larger.img
sed -e 's/\s*\([\+0-9a-zA-Z]*\).*/\1/' << EOF | fdisk piCore-9.0.3-larger.img
    o # clear the in memory partition table
    n # new partition
    p # primary partition
    1 # partition number 1
    8192   #
    77823  #
    n # new partition
    p # primary partition
    2 # partion number 2
    77824  # start immediately after preceding partition
    # default, everything
    p # print the in-memory partition table
    w # write the partition table
    q # and we're done
EOF

mkdir -p card
sudo losetup -o 39845888 /dev/loop0 piCore-9.0.3-larger.img
sudo resize2fs -f /dev/loop0
sudo mount /dev/loop0 card
pushd card/tce/optional
for a in screen git compiletc; do
    sudo wget http://tinycorelinux.net/9.x/armv7/tcz/$a.tcz;
done
popd
sudo umount card
