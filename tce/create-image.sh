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
    t # change type
    c # WIN32 FAT (LBA)
    n # new partition
    p # primary partition
    2 # partion number 2
    77824  # start immediately after preceding partition
    # default, everything
    p # print the in-memory partition table
    w # write the partition table
    q # and we're done
EOF

mkdir -p .extensions-cache
cp *.tcz .extensions-cache
pushd .extensions-cache
for name in `cat ../extensions`; do
    if [ ! -f $name ]; then
        wget http://tinycorelinux.net/9.x/armv7/tcz/$name;
    fi
done
popd

sudo rm -rf mydata.tgz mydata-temp

cp -ar mydata mydata-temp

pushd mydata-temp
sudo chown 0.0 -R opt home var
sudo chown 1001.50 -R home/tc
sudo chown 0.50 -R opt
sudo tar czf ../mydata.tgz ./
popd

mkdir -p card

sudo losetup -o 4194304 /dev/loop0 piCore-9.0.3-larger.img
sudo mount /dev/loop0 card
sudo cp cmdline3.txt card
sudo umount card
sudo losetup -d /dev/loop0

sudo losetup -o 39845888 /dev/loop0 piCore-9.0.3-larger.img
sudo resize2fs -f /dev/loop0
sudo mount /dev/loop0 card
sudo touch card/tce/copy2fs.flg
sudo cp -ar .extensions-cache/* card/tce/optional
sudo cp onboot.lst card/tce
sudo cp mydata.tgz card/tce
sudo ls -alh card/tce
sudo umount card
sudo losetup -d /dev/loop0
