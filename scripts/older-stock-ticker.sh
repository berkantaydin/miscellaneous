#!/bin/bash

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

sigint()
{
	tput sgr0
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

while :;
do
        printline=$bldblu"UUID\tMarket\tSymbol\tPrice\tChange\t% Chg.\tD/T Last Trade\t<<<<< After Hours Trading >>>>>\tSymbol\n"
	for stock in $@
	do
		info=( `wget -O - http://finance.google.com/finance/info?q=$stock 2>/dev/null` )
		n=${#info[@]}
		if [ $n -eq 0 ]; then
			printline="$printline$bldblu$invalidmsg$stock\n"
			continue
		fi
		
		cols=`tput cols`
		
		gid=`echo \"${info[4]:1:-1}\" | sha1sum -`
		gid=`trun ${gid:2}`
		symb=`trun ${info[7]:1:-1}`
		mkt=`trun ${info[10]:1:-1}`
		price=`trun ${info[13]:1:-1}`
		price_cur=`trun ${info[16]:1:-1}`
		unk1=${info[18]:1:-1}
		last_trade_time=${info[19]:8:-2}
		last_trade_tzone=${info[20]:0:-1}
		last_trade_month=${info[23]:1}
		last_trade_day=${info[24]:0:-1}
		price_chg=${info[29]:1:-1}
		price_pchg=${info[32]:1:-1}
		color=${info[35]:3:1}
		
		if [ $n -gt 40 ]; then
			ah_price=${info[37]:1:-1}
			ah_price_cur=${info[39]:1:-1}
			ah_last_trade_month=${info[42]:1:-1}
			ah_last_trade_day=${info[43]:0}
			ah_last_trade_time=${info[44]}
			ah_last_trade_tzone=${info[45]:0:-1}
			ah_price_chg=${info[48]:1:-1}
			ah_price_pchg=${info[51]:1:-1}
			ah_color=${info[54]:3:1}
			dividend=${info[57]:1:-1}
			yield=${info[60]:1:-1}
		fi

		if [ $color == 'g' ]; then termcol="$bldgrn"; fi
		if [ $color == 'r' ]; then termcol="$bldred"; fi		
		if [ $price_chg == "0.00" ]; then termcol="$bldwht"; fi

		if [ $n -gt 40 ]; then
			if [ $ah_color == 'g' ]; then ah_termcol="$bldgrn"; fi
			if [ $ah_color == 'r' ]; then ah_termcol="$bldred"; fi
			if [ $ah_price_chg == "0.00" ]; then ah_termcol="$bldwht"; fi
			ah_info_print="\t$ah_termcol$ah_price\t$ah_price_chg\t$ah_price_pchg\t$ah_last_trade_time"
		else
			ah_info_print="\t\t\t\t"
		fi

		printline="$printline$termcol$gid\t$mkt\t$symb\t$price\t$price_chg\t$price_pchg%\t$last_trade_month $last_trade_day $last_trade_time $last_trade_tzone$ah_info_print\t$termcol$symb\n"
		ah_info_print=""
	done

	clear
	echo -en "$printline"
	sleep 1
done
