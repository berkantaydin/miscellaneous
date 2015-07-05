#!/bin/bash

IMGSTR="$1"

tmux split-window -h "sleep 2; socat /tmp/console-$IMGSTR -,raw,icanon=0,echo=0" &

sleep 2
socat /tmp/hvtty-$IMGSTR -,raw,icanon=0,echo=0
read
