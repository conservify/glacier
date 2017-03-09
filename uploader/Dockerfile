FROM armv7/armhf-ubuntu:16.04

RUN apt-get update && \
    apt-get install -y apt-utils curl openjdk-8-jdk && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

COPY uploader-cli/build/distributions/uploader-cli-1.0-SNAPSHOT.tar /app/uploader-cli.tar

WORKDIR /app

RUN tar xf uploader-cli.tar
RUN mv uploader-cli-* uploader-cli && chmod 755 uploader-cli/bin/uploader-cli && find

CMD ["/app/uploader-cli/bin/uploader-cli"]
