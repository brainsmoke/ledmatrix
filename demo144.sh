#!/bin/sh

stty -F /dev/ttyACM0 raw
python gradient144.py > /dev/ttyACM0
