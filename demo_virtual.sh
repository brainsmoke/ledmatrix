#!/bin/sh
[ -e banner ] || mkfifo banner
[ -p banner ] || (echo "cannot create named pipe")
echo -n '' > banner &
python2 space.py < banner | python2 virtual/virtualbar.py 120 8
