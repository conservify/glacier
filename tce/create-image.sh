#!/bin/bash

set -ex

source functions.sh

# ==============================================================================

CARD0_LODGE_WORK=$BUILD/card0-lodge
CARD0_LODGE_BASE_WORK=$BUILD/card0-lodge-base
sudo rm -rf $CARD0_LODGE_WORK
unarchive_directory $BUILD/card0.gz $CARD0_LODGE_WORK

sudo rm -rf $CARD0_LODGE_BASE_WORK
mkdir $CARD0_LODGE_BASE_WORK
pushd $CARD0_LODGE_BASE_WORK
zcat $CARD0_LODGE_WORK/9.0.3v7.gz | sudo cpio -d -i
sudo patch etc/init.d/tc-config -i $PROJECT/tc-config-rsyslogd.patch
for a in $PROJECT/mandatory/*.tcz; do
    if [ -f $a ]; then
        sudo unsquashfs -f -d ./ $a;
    fi
done
sudo cp $PROJECT/mandatory/etc/rsyslog.conf.lodge etc/rsyslog.conf
sudo cp $PROJECT/mandatory/etc/logrotate.conf etc
sudo cp -ar $PROJECT/mandatory/etc/periodic etc
sudo cp etc/periodic/offsite-backup etc/periodic/13min
sudo cp -ar /etc/ssl etc
sudo rmdir var/log
sudo ln -sf /mnt/mmcblk0p2/data/log var/log
sudo ldconfig -r $CARD0_LODGE_BASE_WORK
sudo mkdir -p home/tc
sudo chmod 755 home/tc
sudo -- sh -c "find | cpio -o -H newc | gzip -9 > $CARD0_LODGE_WORK/9.0.3v7.gz"
popd
sudo rm -rf $CARD0_LODGE_BASE_WORK

for a in $CARD0_LODGE_WORK/cmdline*; do
    sed -i -e 's/quiet/nodhcp cron syslog host=lodge logo.nologo/g' $a
    sed -i -e 's/loglevel=3/loglevel=6/g' $a
done

echo 'dtparam=watchdog=on' | sudo tee --append $CARD0_LODGE_WORK/config.txt

archive_directory $CARD0_LODGE_WORK $BUILD/card0-lodge.gz

# ==============================================================================

CARD0_GLACIER_WORK=$BUILD/card0-glacier
CARD0_GLACIER_BASE_WORK=$BUILD/card0-glacier-base
sudo rm -rf $CARD0_GLACIER_WORK
mkdir -p $CARD0_GLACIER_WORK
unarchive_directory $BUILD/card0.gz $CARD0_GLACIER_WORK

sudo rm -rf $CARD0_GLACIER_BASE_WORK
mkdir $CARD0_GLACIER_BASE_WORK
pushd $CARD0_GLACIER_BASE_WORK
zcat $CARD0_GLACIER_WORK/9.0.3v7.gz | sudo cpio -d -i
sudo patch etc/init.d/tc-config -i $PROJECT/tc-config-rsyslogd.patch
for a in $PROJECT/mandatory/*.tcz; do
    if [ -f $a ]; then
        sudo unsquashfs -f -d ./ $a;
    fi
done
sudo cp $PROJECT/mandatory/etc/rsyslog.conf.glacier etc/rsyslog.conf
sudo cp $PROJECT/mandatory/etc/logrotate.conf etc
sudo cp -ar $PROJECT/mandatory/etc/periodic etc
sudo cp etc/periodic/mirror-obsidian etc/periodic/5min
sudo cp etc/periodic/data-roller etc/periodic/5min
sudo cp -ar /etc/ssl etc
sudo rmdir var/log
sudo ln -sf /mnt/mmcblk0p2/data/log var/log
sudo ldconfig -r $CARD0_GLACIER_BASE_WORK
sudo mkdir -p home/tc
sudo chmod 755 home/tc
sudo -- sh -c "find | cpio -o -H newc | gzip -9 > $CARD0_GLACIER_WORK/9.0.3v7.gz"
popd
sudo rm -rf $CARD0_GLACIER_BASE_WORK

for a in $CARD0_GLACIER_WORK/cmdline*; do
    sed -i -e 's/quiet/nodhcp cron syslog host=glacier logo.nologo/g' $a
    sed -i -e 's/loglevel=3/loglevel=6/g' $a
done

echo 'dtparam=watchdog=on' | sudo tee --append $CARD0_GLACIER_WORK/config.txt

archive_directory $CARD0_GLACIER_WORK $BUILD/card0-glacier.gz

# ==============================================================================

CARD1_WORK=$BUILD/card1
sudo rm -rf $CARD1_WORK
mkdir -p $CARD1_WORK
unarchive_directory $BUILD/card1.gz $CARD1_WORK
sudo touch $CARD1_WORK/tce/copy2fs.flg
sudo cp -ar $PROJECT/.extensions-cache/* $CARD1_WORK/tce/optional
sudo mkdir -p $CARD1_WORK/data/geophone
sudo mkdir -p $CARD1_WORK/data/obsidian
sudo mkdir -p $CARD1_WORK/data/log
sudo cp $PROJECT/onboot.lst $CARD1_WORK/tce
sudo cp $BUILD/mydata.tgz $CARD1_WORK/tce
archive_directory $CARD1_WORK $BUILD/card1-both.gz
