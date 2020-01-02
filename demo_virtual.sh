#!/bin/sh
[ -e banner ] || mkfifo banner
[ -p banner ] || (echo "cannot create named pipe")
echo -n '' > banner &
python3 space.py < banner | python3 virtual/virtualbar.py 120 8
