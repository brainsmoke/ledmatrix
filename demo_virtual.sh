#!/bin/sh
[ -e banner ] || mkfifo banner
[ -p banner ] || (echo "cannot create named pipe")
cat < banner | python2 space.py | python2 virtual/virtualbar.py 120 8
