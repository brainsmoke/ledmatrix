#!/bin/sh

stty -F /dev/ttyACM0 raw
python3 gradient144.py > /dev/ttyACM0
