FROM armv7/armhf-ubuntu

RUN apt-get update && apt-get install -y build-essential cmake lua5.1 liblua5.1-0-dev git golang
WORKDIR /app

RUN git clone https://github.com/axkibe/lsyncd.git
RUN mkdir lsyncd/build && cd lsyncd/build && cmake ../ && make

ADD wrapper.go /app
RUN go build wrapper.go
ADD lsync.conf /app
ADD .ssh /root/.ssh
RUN chmod 600 /root/.ssh/id_rsa

CMD ["./lsyncd/build/lsyncd", "/app/lsync.conf"]
