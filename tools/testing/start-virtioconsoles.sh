#!/bin/bash

IMGSTR="$1"

tmux split-window -h "socat UNIX-LISTEN:/tmp/login-$IMGSTR -,raw,icanon=0,echo=0" &

socat UNIX-LISTEN:/tmp/console-$IMGSTR -,raw,icanon=0,echo=0
read
