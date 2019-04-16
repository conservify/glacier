#!/bin/bash

set -xe

convert glacier-28d.png glacier-6mo.png -append glacier.png
convert lodge-28d.png lodge-6mo.png -append lodge.png
convert lodge.png glacier.png +append battery.png

