#!/bin/bash

for src in "$@"; do
	ffmpeg -i "$src" -vn -ar 44100 -ac 2 -ab 192 -f mp3 -acodec libmp3lame "$src".mp3
done
