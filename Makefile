BUILD ?= build
GOOS ?= linux
GOARCH ?= amd64
GO ?= env GOOS=$(GOOS) GOARCH=$(GOARCH) go

all: $(BUILD) $(BUILD)/render-ascii $(BUILD)/render-archives

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/render-ascii: rendering/*.cpp
	$(CXX) -g -std=c++11 -Wall -o $@ $^ -lm -lpthread -lncurses

$(BUILD)/render-archives: rendering/*.go
	$(GO) build -o $@ $^

clean:
	rm -f $(BUILD)
