#!/bin/sh

stty -F /dev/ttyACM0 raw
[ -e banner ] || mkfifo banner
[ -p banner ] || (echo "cannot create named pipe")
cat < banner | python space.py > /dev/ttyACM0
