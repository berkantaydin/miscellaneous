#!/bin/bash

num="`find | grep -P "\.flac$|\.ogg$|\.m4a$|\.wav$|\.ape$" | wc -l`"
fini="0"

nlc="`echo -ne "\n"`"
while IFS="$nlc" read -ra FILES; do
	for i in "${FILES[@]}"; do
		bname="`basename "$i"`"
		path="${i:0:$[${#i} - ${#bname}]}"
		ffmpeg -loglevel error -i "$i" -f mp3 "$path$bname.mp3" < /dev/null
		rm "$i"
		fini=$[$fini + 1]
		echo "$fini/$num files processed ($bname)"
	done
done <<< "`find | grep -P "\.flac$|\.ogg$|\.m4a$|\.wav$|\.ape$"`"
