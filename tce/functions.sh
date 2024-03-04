#!/bin/bash

PROJECT=`pwd`
BUILD=$PROJECT/build

function archive_directory {
    pushd $1
    sudo find | sudo cpio -o -H newc | gzip -9 > $2
    popd
}

function unarchive_directory {
    mkdir -p $2
    pushd $2
    zcat $1 | sudo cpio -d -i
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

        sudo losetup -o $offs /dev/loop1 $SOURCE
        sudo mount /dev/loop1 $TEMP
        archive_directory $TEMP $BUILD/card$number.gz
        sudo umount $TEMP
        sudo losetup -d /dev/loop1
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

        sudo losetup -o $offs /dev/loop1 $IMAGE
        eval "$FORMAT /dev/loop1"
        mkdir -p card-temp
        sudo mount /dev/loop1 card-temp
        unarchive_directory $FILE card-temp
        sudo umount card-temp
        sudo losetup -d /dev/loop1
        rmdir card-temp
    done
}

function partition_image {
    IMAGE=$1
    dd if=/dev/zero bs=1M count=512 > $IMAGE
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
