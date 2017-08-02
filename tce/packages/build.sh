set -ex

# sudo apt-get install libgcrypt-dev
BUILD=`pwd`/build

mkdir -p $BUILD

if [ ! -f rsyslog-8.28.0.tar.gz ]; then
	wget http://www.rsyslog.com/files/download/rsyslog/rsyslog-8.28.0.tar.gz
fi

if [ ! -f v0.1.10.tar.gz ]; then
	wget https://github.com/rsyslog/libestr/archive/v0.1.10.tar.gz
fi

if [ ! -f v0.99.6.tar.gz ]; then
	wget https://github.com/rsyslog/libfastjson/archive/v0.99.6.tar.gz
fi

if [ ! -f v1.0.6.tar.gz ]; then
	wget https://github.com/rsyslog/liblogging/archive/v1.0.6.tar.gz
fi

if [ ! -f logrotate-3.12.3.tar.xz ]; then
    wget https://github.com/logrotate/logrotate/releases/download/3.12.3/logrotate-3.12.3.tar.xz
fi

pushd $BUILD
tar zxf ../rsyslog-8.28.0.tar.gz
tar zxf ../v0.1.10.tar.gz
tar zxf ../v0.99.6.tar.gz
tar zxf ../v1.0.6.tar.gz
tar zxf ../logrotate-3.12.3.tar.xz
popd

pushd $BUILD/libestr-0.1.10
libtoolize --force && aclocal && autoheader && automake --force-missing --add-missing && autoconf
./configure --prefix=/usr/local
make
make install DESTDIR=$BUILD/libestr-install
popd

pushd $BUILD/libfastjson-0.99.6
libtoolize --force && aclocal && autoheader && automake --force-missing --add-missing && autoconf
./configure --prefix=/usr/local
make
make install DESTDIR=$BUILD/libfastjson-install
popd

pushd $BUILD/liblogging-1.0.6
./autogen.sh --prefix=/usr/local --disable-man-pages
make
make install DESTDIR=$BUILD/liblogging-install
popd

pushd $BUILD/rsyslog-8.28.0
env LDFLAGS="-L$BUILD/libfastjson-install/usr/local/lib -L$BUILD/libestr-install/usr/local/lib -L$BUILD/liblogging-install/usr/local/lib" CFLAGS="-I$BUILD/liblogging-install/usr/local/include -I$BUILD/libestr-install/usr/local/include -I$BUILD/libfastjson-install/usr/local/include/libfastjson" PKG_CONFIG_PATH=$BUILD/libestr-install/usr/local/lib/pkgconfig:$BUILD/libfastjson-install/usr/local/lib/pkgconfig:$BUILD/liblogging-install/usr/local/lib/pkgconfig ./configure --prefix=/usr/local  --disable-uuid --enable-inet --enable-rsyslogd  --enable-imptcp --enable-klog --with-systemdsystemunitdir=$BUILD/rsyslog-install/systemd
make
make install DESTDIR=$BUILD/rsyslog-install
popd

pushd $BUILD/logrotate-3.12.3.
./configure
make
make install DESTDIR=$BUILD/logrotate-install
popd

pushd $BUILD/rsyslog-install
mkdir -p etc
cp -ar ../../rsyslogd/* etc
mkdir -p var/spool/rsyslog
popd

rm -rf $BUILD/*.tcz

mksquashfs $BUILD/libestr-install $BUILD/libestr.tcz -b 4k -no-xattrs
mksquashfs $BUILD/libfastjson-install $BUILD/libfastjson.tcz -b 4k -no-xattrs
mksquashfs $BUILD/liblogging-install $BUILD/liblogging.tcz -b 4k -no-xattrs
mksquashfs $BUILD/rsyslog-install $BUILD/rsyslog.tcz -b 4k -no-xattrs
mksquashfs $BUILD/logrotate-install $BUILD/logrotate.tcz -b 4k -no-xattrs
