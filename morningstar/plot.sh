#!/bin/bash

set -xe

go build add-unix-time.go
go build slack-upload-file.go

./add-unix-time morningstar-lodge.csv > morningstar.csv
gnuplot < plot.gnuplot > lodge.png

./add-unix-time morningstar-glacier.csv > morningstar.csv
gnuplot < plot.gnuplot > glacier.png

rm morningstar.csv

source ./env

./slack-upload-file --token $SLACK_TOKEN --channel glacier --file lodge.png
./slack-upload-file --token $SLACK_TOKEN --channel glacier --file glacier.png
