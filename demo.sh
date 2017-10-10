#!/bin/sh

stty -F /dev/ttyACM0 raw
python space.py > /dev/ttyACM0
