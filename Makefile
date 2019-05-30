GOARCH ?= amd64
GOOS ?= linux
GO ?= env GOOS=$(GOOS) GOARCH=$(GOARCH) go
BUILD ?= build
BUILDARCH ?= $(BUILD)/$(GOOS)-$(GOARCH)

all: $(BUILD) $(BUILD)/render-ascii linux-amd64

linux-amd64:
	GOOS=linux GOARCH=amd64 make go-binaries

linux-arm:
	GOOS=linux GOARCH=arm make go-binaries

darwin-amd64:
	GOOS=darwin GOARCH=amd64 make go-binaries

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/render-ascii: rendering/*.cpp
	$(CXX) -g -std=c++11 -Wall -o $@ $^ -lm -lpthread -lncurses

go-binaries: $(BUILDARCH)/render-archives $(BUILDARCH)/rockblock $(BUILDARCH)/tunneller $(BUILDARCH)/resilience $(BUILDARCH)/uploader $(BUILDARCH)/data-roller $(BUILDARCH)/morningstar

$(BUILDARCH)/render-archives: rendering/*.go
	$(GO) build -o $@ $^

$(BUILDARCH)/rockblock: rockblock/*.go
	$(GO) build -o $@ $^

$(BUILDARCH)/tunneller: tunneller/*.go
	$(GO) build -o $@ $^

$(BUILDARCH)/resilience: resilience/*.go
	$(GO) build -o $@ $^

$(BUILDARCH)/uploader: uploader/*.go
	$(GO) build -o $@ $^

$(BUILDARCH)/data-roller: data-roller/*.go
	$(GO) build -o $@ $^

$(BUILDARCH)/morningstar: morningstar/morningstar.go
	$(GO) build -o $@ $^

deploy: all-arch
	rsync -zvua --progress rendering/static $(BUILD)/linux-arm/render-archives tc@glacier:card/rendering

clean:
	rm -rf $(BUILD)
