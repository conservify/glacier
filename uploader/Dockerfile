FROM conservify/arm-jdk8-oracle

COPY uploader-cli/build/distributions/uploader-cli-1.0-SNAPSHOT.tar /app/uploader-cli.tar

WORKDIR /app

RUN tar xf uploader-cli.tar
RUN mv uploader-cli-* uploader-cli && chmod 755 uploader-cli/bin/uploader-cli && find
RUN mkdir /app/data

CMD ["/app/uploader-cli/bin/uploader-cli", "--data", "/app/data"]
