#!/bin/bash

SOURCE="code:/svr0/glacier/archive/201809"

pushd data

while /bin/true; do
    rsync -zvua --progress $SOURCE .
    sleep 10
done

popd
