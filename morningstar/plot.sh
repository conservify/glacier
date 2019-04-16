#!/bin/bash

set -xe

env

echo $GOPATH
echo $GOROOT

export GOPATH=~jlewallen/go

cd ~jlewallen/conservify/glacier/morningstar

scp ubuntu@code.conservify.org:/var/log/morningstar.log .

grep READING morningstar.log > morningstar-all.csv

(cat header.csv; grep glacier morningstar-all.csv) > morningstar-glacier.csv
(cat header.csv; grep lodge morningstar-all.csv) > morningstar-lodge.csv

grep glacier morningstar-all.csv > morningstar-glacier.csv
grep lodge morningstar-all.csv > morningstar-lodge.csv

go build add-unix-time.go
go build slack-upload-file.go

./add-unix-time --days-to-keep 28 morningstar-lodge.csv > morningstar.csv
gnuplot -e "title='Lodge Voltage (28 days)'" plot.gnuplot > lodge-28d.png

./add-unix-time --days-to-keep 28 morningstar-glacier.csv > morningstar.csv
gnuplot -e "title='Glacier Voltage (28 days)'" plot.gnuplot > glacier-28d.png

./add-unix-time --days-to-keep 180 morningstar-lodge.csv > morningstar.csv
gnuplot -e "title='Lodge Voltage (6 months)'" plot.gnuplot > lodge-6mo.png

./add-unix-time --days-to-keep 180 morningstar-glacier.csv > morningstar.csv
gnuplot -e "title='Glacier Voltage (6 months)'" plot.gnuplot > glacier-6mo.png

rm morningstar.csv morningstar-all.csv

source ./env
CHANNEL=glacier

chown -R jlewallen. $PWD

convert glacier-28d.png glacier-6mo.png -append glacier.png
convert lodge-28d.png lodge-6mo.png -append lodge.png
convert lodge.png glacier.png +append battery.png

# ./slack-upload-file --token $SLACK_TOKEN --channel $CHANNEL --file lodge-28d.png
# ./slack-upload-file --token $SLACK_TOKEN --channel $CHANNEL --file glacier-28d.png

# ./slack-upload-file --token $SLACK_TOKEN --channel $CHANNEL --file lodge-6mo.png
# ./slack-upload-file --token $SLACK_TOKEN --channel $CHANNEL --file glacier-6mo.png

./slack-upload-file --token $SLACK_TOKEN --channel $CHANNEL --file battery.png
