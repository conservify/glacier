FROM armv7/armhf-ubuntu

RUN apt-get update && apt-get install -y golang git
WORKDIR /app

ADD *.go /app/
RUN mkdir /app/data
ENV GOROOT=/usr/lib/go
ENV GOPATH=/app

RUN go get -u github.com/fsnotify/fsnotify
RUN go get -u golang.org/x/sys/...
RUN go build -o uploader *.go

CMD ["/app/uploader"]
