BUILD=build/conservify-glacier-tools

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

godeps:
	go get golang.org/x/crypto/ssh
	go get github.com/tatsushid/go-fastping
	go get github.com/fsnotify/fsnotify
	go get golang.org/x/sys/...
	go get github.com/docker/docker/api
	go get github.com/docker/docker/client
	go get github.com/nlopes/slack
	go get github.com/goburrow/modbus

build/zerotier.tcz: setup
	mksquashfs packages/zerotier build/zerotier.tcz -b 4k -no-xattrs -noappend

build/hamachi.tcz: setup
	sudo rm -rf packages/hamachi/dev/net
	mkdir -p packages/hamachi/dev/net
	mkdir -p packages/hamachi/lib
	sudo mknod packages/hamachi/dev/net/tun c 10 200
	sudo chmod 0666 packages/hamachi/dev/net/tun
	ln -sf ld-linux-armhf.so.3 packages/hamachi/lib/ld-linux.so.3
	ln -sf hamachid packages/hamachi/usr/local/bin/hamachi
	mksquashfs packages/hamachi build/hamachi.tcz -b 4k -no-xattrs -noappend

build/conservify-glacier-tools.tcz: gotools $(BUILD)/app/adc
	cp adc-wrapper $(BUILD)/app/adc-wrapper
	cp uploader-wrapper $(BUILD)/app/uploader-wrapper
	cp tunneller-wrapper $(BUILD)/app/tunneller-wrapper
	cp conservify-glacier-tools $(BUILD)/app
	mksquashfs $(BUILD) build/conservify-glacier-tools.tcz -b 4k -no-xattrs -noappend

packages: setup build/zerotier.tcz build/hamachi.tcz build/conservify-glacier-tools.tcz
	mkdir -p .extensions-cache
	cp build/*.tcz .extensions-cache

piCore-9.0.3.img:
	wget http://tinycorelinux.net/9.x/armv7/releases/RPi/piCore-9.0.3.zip
	unzip piCore-9.0.3.zip

build/lodge.img: piCore-9.0.3.img packages
	bash create-image.sh

images: build/lodge.img build/glacier.img

clean:
	rm -rf $(BUILD)
