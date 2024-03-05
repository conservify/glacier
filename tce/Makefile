PROJECT=$(shell pwd)
BUILD=$(PROJECT)/build
EXTENSIONS_CACHE=".extensions-cache"
MYDATA_ROOT=$(BUILD)/mydata-temp
TOOLS_ROOT=$(BUILD)/working-conservify-glacier-tools
SYNCTHING_ROOT=$(BUILD)/working-syncthing
GO=env GOOS=linux GOARCH=arm go
MAKE=env TOOLCHAIN_PREFIX=arm-linux-gnueabi- CPPFLAGS_EXTRA=-I/usr/arm-linux-gnueabi/include LDFLAGS_EXTRA=-L/usr/arm-linux-gnueabi/lib make
PI_CORE_VERSION = 14.1.0

all: images

setup:
	mkdir -p $(TOOLS_ROOT)/app

image-setup: setup
	sudo losetup -D

$(TOOLS_ROOT)/app/tunneller: ../tunneller/*.go
	$(GO) build -o $@ $^

$(TOOLS_ROOT)/app/resilience: ../resilience/*.go
	$(GO) build -o $@ $^

$(TOOLS_ROOT)/app/uploader: ../uploader/*.go
	$(GO) build -o $@ $^

$(TOOLS_ROOT)/app/morningstar: ../morningstar/morningstar.go
	$(GO) build -o $@ $^

tools: setup $(TOOLS_ROOT)/app/tunneller $(TOOLS_ROOT)/app/resilience $(TOOLS_ROOT)/app/uploader $(TOOLS_ROOT)/app/morningstar 

$(BUILD)/conservify-glacier-tools.tcz: tools
	mkdir -p $(TOOLS_ROOT)/etc
	mkdir -p $(TOOLS_ROOT)/opt
	mkdir -p $(TOOLS_ROOT)/var/spool/rsyslog
	cp -ar mydata/opt/boot*  $(TOOLS_ROOT)/opt
	cp -ar mydata/opt/lodge $(TOOLS_ROOT)/opt
	cp -ar mydata/opt/glacier $(TOOLS_ROOT)/opt
	cp -ar mydata/etc/logrotate.conf $(TOOLS_ROOT)/etc
	cp -ar mydata/etc/rsyslog.conf.* $(TOOLS_ROOT)/etc
	mkdir -p $(TOOLS_ROOT)/usr/local/go/lib/time
	cp $(GOROOT)/lib/time/zoneinfo.zip $(TOOLS_ROOT)/usr/local/go/lib/time
	mksquashfs $(TOOLS_ROOT) $(BUILD)/conservify-glacier-tools.tcz -b 4k -no-xattrs -noappend
	find $(TOOLS_ROOT)

$(BUILD)/mydata.tgz: setup
	sudo rm -rf mydata.tgz $(MYDATA_ROOT)
	cp -ar mydata $(MYDATA_ROOT)
	mkdir $(MYDATA_ROOT)/home/tc/.ssh -p
	# cp ~/.ssh/id_rsa* $(MYDATA_ROOT)/home/tc/.ssh
	cp ~/.ssh/id_rsa.pub $(MYDATA_ROOT)/home/tc/.ssh/authorized_keys
	chmod 755 $(MYDATA_ROOT)/home/tc
	chmod 700 $(MYDATA_ROOT)/home/tc/.ssh
	# chmod 600 $(MYDATA_ROOT)/home/tc/.ssh/id_rsa
	# chmod 644 $(MYDATA_ROOT)/home/tc/.ssh/id_rsa.pub
	chmod 644 $(MYDATA_ROOT)/home/tc/.ssh/authorized_keys
	sudo chown 1001.50 -R $(MYDATA_ROOT)/home/tc

	mkdir $(MYDATA_ROOT)/root/.ssh -p
	# cp ~/.ssh/id_rsa* $(MYDATA_ROOT)/root/.ssh
	cp ~/.ssh/id_rsa.pub $(MYDATA_ROOT)/root/.ssh/authorized_keys
	chmod 755 $(MYDATA_ROOT)/root
	chmod 700 $(MYDATA_ROOT)/root/.ssh
	# chmod 600 $(MYDATA_ROOT)/root/.ssh/id_rsa
	# chmod 644 $(MYDATA_ROOT)/root/.ssh/id_rsa.pub
	chmod 644 $(MYDATA_ROOT)/root/.ssh/authorized_keys
	sudo chown 0.0 -R $(MYDATA_ROOT)/root

	sudo chown 0.0 $(MYDATA_ROOT)/home
	sudo chown 0.0 -R $(MYDATA_ROOT)/opt $(MYDATA_ROOT)/var
	sudo chown 0.50 -R $(MYDATA_ROOT)/opt
	(cd $(MYDATA_ROOT) && sudo tar czf $(BUILD)/mydata.tgz ./)

godeps:
	go get golang.org/x/crypto/ssh
	go get github.com/tatsushid/go-fastping
	go get github.com/fsnotify/fsnotify
	go get golang.org/x/sys/...
	go get github.com/docker/docker/api
	go get github.com/docker/docker/client
	go get github.com/nlopes/slack
	go get github.com/goburrow/modbus
	go get github.com/jpillora/backoff

packages: setup $(BUILD)/syncthing.tcz $(BUILD)/conservify-glacier-tools.tcz
	mkdir -p $(EXTENSIONS_CACHE)
	echo These are made on another Pi.
	cp packages/*.tcz $(BUILD)
	cp $(BUILD)/*.tcz $(EXTENSIONS_CACHE)
	for name in `cat extensions`; do                                                                    \
		if [ ! -f $(EXTENSIONS_CACHE)/$$name ]; then                                                    \
			(cd $(EXTENSIONS_CACHE) && wget -cq http://tinycorelinux.net/14.x/armv7/tcz/$$name.md5.txt); \
			(cd $(EXTENSIONS_CACHE) && wget -cq http://tinycorelinux.net/14.x/armv7/tcz/$$name.dep);     \
			(cd $(EXTENSIONS_CACHE) && wget -cq http://tinycorelinux.net/14.x/armv7/tcz/$$name);         \
		fi                                                                                              \
	done

piCore-$(PI_CORE_VERSION).img:
	wget http://tinycorelinux.net/14.x/armv6/releases/RPi/piCore-$(PI_CORE_VERSION).zip
	unzip piCore-$(PI_CORE_VERSION).zip
	rm -f README piCore-*.txt

syncthing-linux-arm-v1.27.3.tar.gz:
	wget https://github.com/syncthing/syncthing/releases/download/v1.27.3/syncthing-linux-arm-v1.27.3.tar.gz

$(BUILD)/syncthing.tcz: syncthing-linux-arm-v1.27.3.tar.gz
	rm -rf $(SYNCTHING_ROOT)
	mkdir -p $(SYNCTHING_ROOT)/etc
	mkdir -p $(SYNCTHING_ROOT)/opt
	cd $(SYNCTHING_ROOT)/opt && tar xf ../../../syncthing-linux-arm-v1.27.3.tar.gz
	mv $(SYNCTHING_ROOT)/opt/syncthing-linux-arm* $(SYNCTHING_ROOT)/opt/syncthing
	mksquashfs $(SYNCTHING_ROOT) $(BUILD)/syncthing.tcz -b 4k -no-xattrs -noappend

$(BUILD)/card0.gz $(BUILD)/card1.gz: piCore-$(PI_CORE_VERSION).img
	bash ./archive-partitions.sh piCore-$(PI_CORE_VERSION).img

$(BUILD)/card0-lodge.gz $(BUILD)/card0-glacier.gz $(BUILD)/card1-both.gz: image-setup $(BUILD)/card0.gz $(BUILD)/card1.gz $(BUILD)/mydata.tgz packages
	bash create-image.sh

$(BUILD)/lodge.img: $(BUILD)/card0-lodge.gz $(BUILD)/card0-glacier.gz $(BUILD)/card1-both.gz
	./partition-image.sh $(BUILD)/lodge.img
	./fill-partition.sh $(BUILD)/lodge.img "W95" $(BUILD)/card0-lodge.gz "sudo mkfs.vfat"
	./fill-partition.sh $(BUILD)/lodge.img "Linux" $(BUILD)/card1-both.gz "sudo mkfs.ext4"

$(BUILD)/glacier.img: $(BUILD)/card0-glacier.gz $(BUILD)/card1-both.gz
	./partition-image.sh $(BUILD)/glacier.img
	./fill-partition.sh $(BUILD)/glacier.img "W95" $(BUILD)/card0-glacier.gz "sudo mkfs.vfat"
	./fill-partition.sh $(BUILD)/glacier.img "Linux" $(BUILD)/card1-both.gz "sudo mkfs.ext4"

images: $(BUILD)/lodge.img $(BUILD)/glacier.img

clean:
	sudo rm -rf $(BUILD)
