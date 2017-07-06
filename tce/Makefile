BUILD=build

all: image
	mkdir -p $(BUILD)/app
	mkdir -p $(BUILD)/app/data
	go build -o $(BUILD)/app/tunneller ../tunneller/*.go
	go build -o $(BUILD)/app/resilience ../resilience/*.go
	go build -o $(BUILD)/app/status ../status/*.go
	go build -o $(BUILD)/app/uploader ../uploader/*.go
	go build -o $(BUILD)/app/morningstar ../morningstar/*.go
	cd ../psn-adc && make
	cp ../psn-adc/AdcDemo $(BUILD)/app/adc
	cp adc-wrapper $(BUILD)/app/adc-wrapper
	cp uploader-wrapper $(BUILD)/app/uploader-wrapper
	cp conservify-glacier-tools $(BUILD)/app
	mksquashfs $(BUILD) conservify-glacier-tools.tcz -b 4k -no-xattrs 

piCore-9.0.3.img:
	wget http://tinycorelinux.net/9.x/armv7/releases/RPi/piCore-9.0.3.zip
	unzip piCore-9.0.3.zip

piCore-9.0.3-larger.img: piCore-9.0.3.img
	bash create-image.sh

image: piCore-9.0.3-larger.img

deps:
	go get golang.org/x/crypto/ssh
	go get github.com/tatsushid/go-fastping
	go get github.com/fsnotify/fsnotify
	go get golang.org/x/sys/...
	go get github.com/docker/docker/api
	go get github.com/docker/docker/client
	go get github.com/nlopes/slack
	go get github.com/goburrow/modbus

clean:
	rm -rf $(BUILD)
