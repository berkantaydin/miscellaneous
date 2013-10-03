#!/bin/sh

# Real-Time stock ticker script, using Google's API
# Copyright (C) 2012 Calvin Owens
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of version 2 of the GNU General Public License as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# version 2 for more details.

# FIXME: Truncate fields to 8 characters (eg, BRK.A, SCHB break display)
#	 Handle invalid stock ticker or blank return, instead of crashing

sigint()
{
        tput sgr0
	echo -ne "\n"
        exit $?
}
trap sigint SIGINT

txtbld=$(tput bold)
bldblu=${txtbld}$(tput setaf 6) #  blue
bldwht=${txtbld}$(tput setaf 7) #  white
bldred=${txtbld}$(tput setaf 1) #  red
bldgrn=${txtbld}$(tput setaf 2) #  green

invalidmsg="No data for symbol "

trun()
{
        tmp=$1
        if [ ${#tmp} -gt 7 ]; then
                tmp="${tmp:0:7}"
        fi
        echo -ne "$tmp"
}

xml="<empty set>"

get_field()
{
	field="<$1 "
	line="$xml"
	line="${line##*$field}"
	line="${line#*\"}"
	line="${line%%\"*}"
	echo -n "$line"
}

set_color()
{
	if [ $1 == "-" ]; then
		echo -ne "$bldred"
	else
		echo -ne "$bldgrn"
	fi
}

nice_volume()
{
	tmp=$1

	if [ ${#tmp} -gt 9 ]; then
		echo -n "${tmp:0:-9} Bln"
		return
	fi
	if [ ${#tmp} -gt 6 ]; then
		echo -n "${tmp:0:-6} Mil"
		return
	fi
	if [ ${#tmp} -gt 3 ]; then
		echo -n "${tmp:0:-3} Tho"
		return
	fi
	if [ $tmp == "000" ]; then
		echo -n " "
		return
	fi
	echo $tmp
}

clear
while :; do
	clear
	echo -ne "${bldblu}"
	date -R
	echo -ne "Market\tSymbol\tLast\t <<<Day Chg>>>\tLast Trade\tP. Clo\tOpen\tDay Lo\tDay Hi\tVolume\tAvg Vol\tMkt Cap\tCompany Name"
	for stock in $@; do
		xml=`wget -O - http://www.google.com/ig/api?stock=$stock 2>/dev/null`
	
		market=`get_field "exchange"`
		last_trade=`get_field "trade_timestamp"`
		symbol=`get_field "symbol"`
		company=`get_field "company"`
		market_cap=`get_field "market_cap"`
		market_cap="${market_cap:0:-3}000000"
		market_cap=`nice_volume $market_cap`
		y_close=`get_field "y_close"`
		open=`get_field "open"`
		last=`get_field "last"`
		high=`get_field "high"`
		low=`get_field "low"`
		change=`get_field "change"`
		perc_change=`get_field "perc_change"`
		volume=`get_field "volume"`
		volume=`nice_volume $volume`
		avg_volume="`get_field "avg_volume"`000"
		avg_volume=`nice_volume $avg_volume`
	
		set_color "${change:0:1}"
		echo -ne "\n$market\t$symbol\t$last\t$change\t$perc_change%\t$last_trade\t$y_close\t$open\t$low\t$high\t$volume\t$avg_volume\t$market_cap\t$company"
	done
	sleep 30
done
