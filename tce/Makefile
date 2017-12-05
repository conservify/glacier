PROJECT=$(shell pwd)
BUILD=$(PROJECT)/build
EXTENSIONS_CACHE=".extensions-cache"
MYDATA_ROOT=$(BUILD)/mydata-temp
TOOLS_ROOT=$(BUILD)/conservify-glacier-tools
GO=env GOOS=linux GOARCH=arm go
MAKE=env TOOLCHAIN_PREFIX=arm-linux-gnueabi- CPPFLAGS_EXTRA=-I/usr/arm-linux-gnueabi/include LDFLAGS_EXTRA=-L/usr/arm-linux-gnueabi/lib make

GO=go
MAKE=make

all: images

fixes: $(BUILD)/conservify-glacier-tools.tcz

setup:
	mkdir -p $(BUILD)/app/data/obsidian
	mkdir -p $(TOOLS_ROOT)/app

image-setup: setup
	sudo losetup -D

$(TOOLS_ROOT)/app/tunneller: ../tunneller/*.go
	$(GO) build -o $@ $^

$(TOOLS_ROOT)/app/resilience: ../resilience/*.go
	$(GO) build -o $@ $^

$(TOOLS_ROOT)/app/uploader: ../uploader/*.go
	$(GO) build -o $@ $^

$(TOOLS_ROOT)/app/data-roller: ../data-roller/*.go
	$(GO) build -o $@ $^

$(TOOLS_ROOT)/app/morningstar: ../morningstar/morningstar.go
	$(GO) build -o $@ $^

$(TOOLS_ROOT)/app/adc:
	cd ../psn-adc && $(MAKE) 
	cp ../psn-adc/AdcDemo $(TOOLS_ROOT)/app/adc

tools: setup $(TOOLS_ROOT)/app/adc $(TOOLS_ROOT)/app/tunneller $(TOOLS_ROOT)/app/resilience $(TOOLS_ROOT)/app/uploader $(TOOLS_ROOT)/app/morningstar $(TOOLS_ROOT)/app/data-roller

patch: $(BUILD)/conservify-glacier-patch-lodge.tcz

$(BUILD)/conservify-glacier-patch-lodge.tcz: tools
	mkdir -p $(TOOLS_ROOT)/usr/local/go/lib/time
	mkdir -p $(TOOLS_ROOT)/etc
	mkdir -p $(TOOLS_ROOT)/opt
	cp mydata/opt/bootlocal.sh $(TOOLS_ROOT)/opt
	cp mandatory/etc/rsyslog.conf.lodge $(TOOLS_ROOT)/etc/rsyslog.conf
	cp $(GOROOT)/lib/time/zoneinfo.zip $(TOOLS_ROOT)/usr/local/go/lib/time
	mksquashfs $(TOOLS_ROOT) $(BUILD)/conservify-glacier-patch-lodge.tcz -b 4k -no-xattrs -noappend
	find $(TOOLS_ROOT)

$(BUILD)/conservify-glacier-tools.tcz: tools
	mkdir -p $(TOOLS_ROOT)/etc
	mkdir -p $(TOOLS_ROOT)/opt
	mkdir -p $(TOOLS_ROOT)/var/spool/rsyslog
	cp mydata/opt/boot*  $(TOOLS_ROOT)/opt
	cp -ar mydata/opt/lodge $(TOOLS_ROOT)/opt
	cp -ar mydata/opt/glacier $(TOOLS_ROOT)/opt
	cp mandatory/etc/logrotate.conf $(TOOLS_ROOT)/etc
	cp mandatory/etc/rsyslog.conf.* $(TOOLS_ROOT)/etc
	mkdir -p $(TOOLS_ROOT)/usr/local/go/lib/time
	cp $(GOROOT)/lib/time/zoneinfo.zip $(TOOLS_ROOT)/usr/local/go/lib/time
	cp adc-wrapper $(TOOLS_ROOT)/app/adc-wrapper
	cp uploader-wrapper $(TOOLS_ROOT)/app/uploader-wrapper
	cp tunneller-wrapper $(TOOLS_ROOT)/app/tunneller-wrapper
	mksquashfs $(TOOLS_ROOT) $(BUILD)/conservify-glacier-tools.tcz -b 4k -no-xattrs -noappend
	find $(TOOLS_ROOT)

$(BUILD)/mydata.tgz: setup
	sudo rm -rf mydata.tgz $(MYDATA_ROOT)
	cp -ar mydata $(MYDATA_ROOT)
	mkdir $(MYDATA_ROOT)/home/tc/.ssh -p
	cp ~/.ssh/id_rsa* $(MYDATA_ROOT)/home/tc/.ssh
	cp ~/.ssh/id_rsa.pub $(MYDATA_ROOT)/home/tc/.ssh/authorized_keys
	chmod 755 $(MYDATA_ROOT)/home/tc
	chmod 700 $(MYDATA_ROOT)/home/tc/.ssh
	chmod 600 $(MYDATA_ROOT)/home/tc/.ssh/id_rsa
	chmod 644 $(MYDATA_ROOT)/home/tc/.ssh/id_rsa.pub
	chmod 644 $(MYDATA_ROOT)/home/tc/.ssh/authorized_keys
	sudo chown 1001.50 -R $(MYDATA_ROOT)/home/tc

	mkdir $(MYDATA_ROOT)/root/.ssh -p
	cp ~/.ssh/id_rsa* $(MYDATA_ROOT)/root/.ssh
	cp ~/.ssh/id_rsa.pub $(MYDATA_ROOT)/root/.ssh/authorized_keys
	chmod 755 $(MYDATA_ROOT)/root
	chmod 700 $(MYDATA_ROOT)/root/.ssh
	chmod 600 $(MYDATA_ROOT)/root/.ssh/id_rsa
	chmod 644 $(MYDATA_ROOT)/root/.ssh/id_rsa.pub
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

$(BUILD)/zerotier.tcz: setup
	mksquashfs packages/zerotier $(BUILD)/zerotier.tcz -b 4k -no-xattrs -noappend

$(BUILD)/hamachi.tcz: setup
	sudo rm -rf packages/hamachi/dev/net
	mkdir -p packages/hamachi/dev/net
	mkdir -p packages/hamachi/lib
	sudo mknod packages/hamachi/dev/net/tun c 10 200
	sudo chmod 0666 packages/hamachi/dev/net/tun
	ln -sf ld-linux-armhf.so.3 packages/hamachi/lib/ld-linux.so.3
	ln -sf hamachid packages/hamachi/usr/local/bin/hamachi
	mksquashfs packages/hamachi $(BUILD)/hamachi.tcz -b 4k -no-xattrs -noappend

$(BUILD)/r8152.tcz $(BUILD)/usbserial.tcz $(BUILD)/logrotate.tcz:
	echo These are made on another Pi.
	cp packages/r8152.tcz $(BUILD)
	cp packages/usbserial.tcz $(BUILD)
	cp packages/logrotate.tcz $(BUILD)

packages: setup $(BUILD)/zerotier.tcz $(BUILD)/hamachi.tcz $(BUILD)/conservify-glacier-tools.tcz $(BUILD)/r8152.tcz $(BUILD)/usbserial.tcz $(BUILD)/logrotate.tcz
	mkdir -p $(EXTENSIONS_CACHE)
	cp $(BUILD)/*.tcz $(EXTENSIONS_CACHE)
	for name in `cat extensions`; do                                                                    \
		if [ ! -f $(EXTENSIONS_CACHE)/$$name ]; then                                                    \
			(cd $(EXTENSIONS_CACHE) && wget -cq http://tinycorelinux.net/9.x/armv7/tcz/$$name.md5.txt); \
			(cd $(EXTENSIONS_CACHE) && wget -cq http://tinycorelinux.net/9.x/armv7/tcz/$$name.dep);     \
			(cd $(EXTENSIONS_CACHE) && wget -cq http://tinycorelinux.net/9.x/armv7/tcz/$$name);         \
		fi                                                                                              \
	done

piCore-9.0.3.img:
	wget http://tinycorelinux.net/9.x/armv7/releases/RPi/piCore-9.0.3.zip
	unzip piCore-9.0.3.zip
	rm -f README piCore-*.txt

$(BUILD)/card0.gz $(BUILD)/card1.gz: piCore-9.0.3.img
	bash ./archive-partitions.sh piCore-9.0.3.img

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
	cd ../psn-adc && make clean
	sudo rm -rf $(BUILD)
