#!/bin/bash

## Script to quickly change timezone on Gentoo when I'm flying around the US

if [ "`whoami`" != "root" ]; then
	echo "Only root can change the timezone"
	exit
fi

cd /usr/share/zoneinfo
select ZONE in US/Pacific US/Mountain US/Central US/Eastern UTC; do
	cp $ZONE /etc/localtime
	echo "$ZONE" > /etc/timezone
	break
done
