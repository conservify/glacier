#!/bin/sh

hamachi login
hamachi attach jacob@conservify.org
sleep 5
hamachi set-nick `hostname`
sleep 5
hamachi do-join 368-533-593 ""
sleep 5
filetool.sh -b
