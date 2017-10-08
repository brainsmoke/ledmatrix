#!/bin/sh

stty -F /dev/ttyACM0 raw
python gradient.py > /dev/ttyACM0
