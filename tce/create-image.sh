#!/bin/bash

set -ex

PROJECT=`pwd`
BUILD=$PROJECT/build

function archive_directory {
    pushd $1
    find | cpio -o -H newc | gzip -9 > $2
    popd
}

function unarchive_directory {
    mkdir -p $2
    pushd $2
    zcat $1 | cpio -d -i
    popd
}

function archive_partitions {
    SOURCE=$1
    TEMP=$BUILD/card
    number=0

    mkdir -p $TEMP
    fdisk -c -u -l -o Device,Start,End,Type $SOURCE | grep $SOURCE | grep -v Disk | while read line
    do
        set -f
        set -- $line
        offs=$(($2*512))

        losetup -o $offs /dev/loop0 $SOURCE
        mount /dev/loop0 $TEMP
        archive_directory $TEMP $BUILD/card$number.gz
        umount $TEMP
        losetup -d /dev/loop0
        number=$((number+1))
    done
}

function fill_partition {
    IMAGE=$1
    FILTER=$2
    FILE=$3
    FORMAT=$4

    fdisk -c -u -l -o Device,Start,End,Type $IMAGE | grep $IMAGE | grep $FILTER | while read line
    do
        set -f
        set -- $line
        offs=$(($2*512))

        losetup -o $offs /dev/loop0 $IMAGE
        eval "$FORMAT /dev/loop0"
        mkdir -p card-temp
        mount /dev/loop0 card-temp
        unarchive_directory $FILE card-temp
        umount card-temp
        losetup -d /dev/loop0
        rmdir card-temp
    done
}

function download_extensions {
    mkdir -p .extensions-cache
    pushd .extensions-cache
    for name in `cat ../extensions`; do
        if [ ! -f $name ]; then
            wget http://tinycorelinux.net/9.x/armv7/tcz/$name;
        fi
    done
    popd
}

function build_mydata {
    WORK=$BUILD/mydata-temp
    rm -rf mydata.tgz $WORK
    cp -ar mydata $WORK
    pushd $WORK
    mkdir home/tc/.ssh -p
    cp ~/.ssh/id_rsa* home/tc/.ssh
    cp ~/.ssh/id_rsa.pub home/tc/.ssh/authorized_keys
    chmod 700 home/tc/.ssh
    chmod 600 home/tc/.ssh/id_rsa
    chmod 644 home/tc/.ssh/id_rsa.pub
    chmod 644 home/tc/.ssh/authorized_keys
    chown 0.0 -R opt home var
    chown 1001.50 -R home/tc
    chown 0.50 -R opt
    tar czf ../mydata.tgz ./
    popd
}

function partition_image {
    IMAGE=$1
    dd if=/dev/zero bs=1M count=200 > $IMAGE
    sed -e 's/\s*\([\+0-9a-zA-Z]*\).*/\1/' << EOF | fdisk $IMAGE
    o # clear the in memory partition table
    n # new partition
    p # primary partition
    1 # partition number 1
    8192   #
    110000  #
    t # change type
    c # WIN32 FAT (LBA)
    a # mark partition 1 bootable
    n # new partition
    p # primary partition
    2 # partion number 2
    110001 # start immediately after preceding partition
    # default, everything
    p # print the in-memory partition table
    w # write the partition table
    q # and we're done
EOF
}

function cleanup {
    losetup -D
}

losetup -D

SOURCE=piCore-9.0.3.img
archive_partitions $SOURCE
download_extensions
build_mydata

WORK=$BUILD/card0
mkdir -p $WORK

rm -rf $BUILD/card0-base
mkdir $BUILD/card0-base

unarchive_directory $BUILD/card0.gz $WORK

pushd $BUILD/card0-base
zcat $WORK/9.0.3v7.gz | cpio -d -i
patch etc/init.d/tc-config -i $PROJECT/tc-config-rsyslogd.patch
for a in $PROJECT/mandatory/*.tcz; do unsquashfs -f -d ./ $a ; done
ldconfig -r $BUILD/card0-base
find | cpio -o -H newc | gzip -9 > $WORK/9.0.3v7.gz
popd

for a in $WORK/cmdline*; do
    sed -i -e 's/quiet/nodhcp cron syslog host=glacier logo.nologo/g' $a
    sed -i -e 's/loglevel=3/loglevel=6/g' $a
done

archive_directory $WORK $BUILD/card0.gz

WORK=$BUILD/card1
rm -rf $WORK
mkdir -p $WORK
unarchive_directory $BUILD/card1.gz $WORK
touch $WORK/tce/copy2fs.flg
cp -ar $PROJECT/.extensions-cache/* $WORK/tce/optional
cp $PROJECT/onboot.lst $WORK/tce
cp $BUILD/mydata.tgz $WORK/tce
archive_directory $WORK $BUILD/card1.gz

IMAGE="$BUILD/test.img"
partition_image $IMAGE
fill_partition $IMAGE "W95" $BUILD/card0.gz "sudo mkfs.vfat"
fill_partition $IMAGE "Linux" $BUILD/card1.gz "sudo mkfs.ext4"

cleanup
