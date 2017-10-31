#!/bin/bash

set -xe

scp ubuntu@code.conservify.org:/var/log/morningstar.log .

grep READING morningstar.log > morningstar-all.csv

(cat header.csv; grep glacier morningstar-all.csv) > morningstar-glacier.csv
(cat header.csv; grep lodge morningstar-all.csv) > morningstar-lodge.csv

grep glacier morningstar-all.csv > morningstar-glacier.csv
grep lodge morningstar-all.csv > morningstar-lodge.csv

go build add-unix-time.go
go build slack-upload-file.go

./add-unix-time morningstar-lodge.csv > morningstar.csv
gnuplot < plot.gnuplot > lodge.png

./add-unix-time morningstar-glacier.csv > morningstar.csv
gnuplot < plot.gnuplot > glacier.png

rm morningstar.csv morningstar-all.csv

source ./env
CHANNEL=glacier

./slack-upload-file --token $SLACK_TOKEN --channel $CHANNEL --file lodge.png
./slack-upload-file --token $SLACK_TOKEN --channel $CHANNEL --file glacier.png
