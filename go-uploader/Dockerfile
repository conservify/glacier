FROM armv7/armhf-ubuntu

RUN apt-get update && apt-get install -y golang
WORKDIR /app

ADD *.go /app
RUN mkdir /app/data
RUN GOROOT=/app go get -u
RUN GOROOT=/app go build -o uploader *.go

CMD ["/app/main", "--data", "/app/data"]
