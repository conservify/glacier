default: build

build:
	${GOROOT}/bin/go build *.go

deps:
	go get -v github.com/jpillora/backoff
	go get -v github.com/go-audio/audio
	go get -v github.com/go-audio/wav

clean:
	rm -f uploader
