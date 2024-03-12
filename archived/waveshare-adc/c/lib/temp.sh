#!/bin/bash

set -xe

for a in *.h *.c; do
	clang-format -style=file $a > .temp
	\mv .temp $a
done
