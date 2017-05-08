FROM armv7/armhf-ubuntu

RUN apt-get update && apt-get install -y build-essential git python udev
ADD ./ /app
WORKDIR /app

RUN make

CMD ["./AdcDemo"]
