#!/bin/sh

stty -F /dev/ttyACM0 raw
[ -e banner ] || mkfifo banner
[ -p banner ] || (echo "cannot create named pipe")
echo -n '' > banner &
python3 space.py < banner > /dev/ttyACM0
