BUILD=build/conservify-glacier-tools

all: image

setup:
	mkdir -p $(BUILD)/app/data/obsidian

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

gotools: $(BUILD)/app/tunneller $(BUILD)/app/resilience $(BUILD)/app/uploader $(BUILD)/app/morningstar

godeps:
	go get golang.org/x/crypto/ssh
	go get github.com/tatsushid/go-fastping
	go get github.com/fsnotify/fsnotify
	go get golang.org/x/sys/...
	go get github.com/docker/docker/api
	go get github.com/docker/docker/client
	go get github.com/nlopes/slack
	go get github.com/goburrow/modbus

conservify-glacier-tools.tcz: gotools $(BUILD)/app/adc
	cp adc-wrapper $(BUILD)/app/adc-wrapper
	cp uploader-wrapper $(BUILD)/app/uploader-wrapper
	cp tunneller-wrapper $(BUILD)/app/tunneller-wrapper
	cp conservify-glacier-tools $(BUILD)/app
	rm -f conservify-glacier-tools.tcz
	mksquashfs $(BUILD) conservify-glacier-tools.tcz -b 4k -no-xattrs
	mkdir -p .extensions-cache
	mv conservify-glacier-tools.tcz .extensions-cache

piCore-9.0.3.img:
	wget http://tinycorelinux.net/9.x/armv7/releases/RPi/piCore-9.0.3.zip
	unzip piCore-9.0.3.zip

build/piCore-9.0.3-lodge.img: piCore-9.0.3.img conservify-glacier-tools.tcz
	bash create-image.sh

image: build/piCore-9.0.3-lodge.img

clean:
	rm -rf $(BUILD)
	rm -rf piCore-*-lodge.img
	rm -rf piCore-*-glacier.img
