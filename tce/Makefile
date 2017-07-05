BUILD=build

all:
	mkdir -p $(BUILD)/app
	go build -o $(BUILD)/app/tunneller ../tunneller/*.go
	go build -o $(BUILD)/app/resilience ../resilience/*.go
	go build -o $(BUILD)/app/status ../status/*.go
	go build -o $(BUILD)/app/uploader ../uploader/*.go
	go build -o $(BUILD)/app/morningstar ../morningstar/*.go
	cd ../psn-adc && make
	cp ../psn-adc/AdcDemo $(BUILD)/app
	mksquashfs $(BUILD) conservify-glacier-tools.tcz -b 4k -no-xattrs 

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
