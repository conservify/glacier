PROJECT=$(shell pwd)
BUILD=$(PROJECT)/build
EXTENSIONS_CACHE=".extensions-cache"
MYDATA_ROOT=$(BUILD)/mydata-temp
TOOLS_ROOT=$(BUILD)/conservify-glacier-tools

all: images

setup:
	sudo losetup -D
	@mkdir -p $(BUILD)/app/data/obsidian

$(BUILD)/app/tunneller: ../tunneller/*.go
	env GOOS=linux GOARCH=arm go build -o $@ $^

$(BUILD)/app/resilience: ../resilience/*.go
	env GOOS=linux GOARCH=arm go build -o $@ $^

$(BUILD)/app/uploader: ../uploader/*.go
	env GOOS=linux GOARCH=arm go build -o $@ $^

$(BUILD)/app/morningstar: ../morningstar/*.go
	env GOOS=linux GOARCH=arm go build -o $@ $^

$(BUILD)/app/adc: setup
	cd ../psn-adc && make
	cp ../psn-adc/AdcDemo $(BUILD)/app/adc

gotools: setup $(BUILD)/app/tunneller $(BUILD)/app/resilience $(BUILD)/app/uploader $(BUILD)/app/morningstar

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
	sudo chown 0.0 -R $(MYDATA_ROOT)/opt $(MYDATA_ROOT)/home $(MYDATA_ROOT)/var
	sudo chown 1001.50 -R $(MYDATA_ROOT)/home/tc
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

$(BUILD)/conservify-glacier-tools.tcz: gotools $(BUILD)/app/adc
	mkdir -p $(TOOLS_ROOT)/app
	cp adc-wrapper $(TOOLS_ROOT)/app/adc-wrapper
	cp uploader-wrapper $(TOOLS_ROOT)/app/uploader-wrapper
	cp tunneller-wrapper $(TOOLS_ROOT)/app/tunneller-wrapper
	cp conservify-glacier-tools $(TOOLS_ROOT)/app
	mksquashfs $(TOOLS_ROOT) $(BUILD)/conservify-glacier-tools.tcz -b 4k -no-xattrs -noappend

$(BUILD)/r8152.tcz $(BUILD)/usbserial.tcz $(BUILD)/logrotate.tcz:
	echo These are made on another Pi.
	cp packages/r8152.tcz $(BUILD)
	cp packages/usbserial.tcz $(BUILD)
	cp packages/logrotate.tcz $(BUILD)

packages: setup $(BUILD)/zerotier.tcz $(BUILD)/hamachi.tcz $(BUILD)/conservify-glacier-tools.tcz $(BUILD)/r8152.tcz $(BUILD)/usbserial.tcz $(BUILD)/logrotate.tcz
	mkdir -p $(EXTENSIONS_CACHE)
	cp $(BUILD)/*.tcz $(EXTENSIONS_CACHE)
	for name in `cat extensions`; do                                                        \
		if [ ! -f $(EXTENSIONS_CACHE)/$$name ]; then                                        \
			(cd $(EXTENSIONS_CACHE) && wget http://tinycorelinux.net/9.x/armv7/tcz/$$name); \
		fi                                                                                  \
	done

piCore-9.0.3.img:
	wget http://tinycorelinux.net/9.x/armv7/releases/RPi/piCore-9.0.3.zip
	unzip piCore-9.0.3.zip
	rm -f README 	piCore-*.txt

$(BUILD)/card0.gz $(BUILD)/card1.gz: piCore-9.0.3.img
	bash ./archive-partitions.sh piCore-9.0.3.img

$(BUILD)/card0-lodge.gz $(BUILD)/card0-glacier.gz $(BUILD)/card1-both.gz: $(BUILD)/card0.gz $(BUILD)/card1.gz $(BUILD)/mydata.tgz packages
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
