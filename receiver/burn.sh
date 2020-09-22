#!/bin/sh
if [ $# -eq 1 ]; then
    avrdude -v -p attiny85 -c arduino -P /dev/ttyACM0 -b 19200 -U flash:w:$1:i
else
    echo "Usage: receiver/burn.sh [hex]"
fi
