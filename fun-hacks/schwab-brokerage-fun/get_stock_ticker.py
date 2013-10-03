#!/usr/bin/python3

## get_stock_ticker.py: Basic stock ticker, using a Schwab brokerage account
##
## This script polls the quote for a stock at a set interval, and outputs the
## difference in volume/price since the last poll. What you end up with is a
## basic list of trades being made, although obviously it will lump multiple
## trades that happen in one window into a single line.
##
## Copyright (C) 2013 Calvin Owens
##
## This program is free software: you can redistribute it and/or modify it under
## the terms of version 2 only of the GNU General Public License as published by 
## the Free Software Foundation.
##
## THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
## FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE COPYRIGHT
## HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
## WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
## CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THIS SOFTWARE.

##########################################
# Put your Schwab username/password here #
##########################################
username = "";
password = "";

# Libraries
import sys;
import re;
import copy;
import http.cookiejar;
import urllib;
import time;
import decimal;
from decimal import Decimal;
from bs4 import BeautifulSoup;

def make_str(*args):
	return ''.join(args);

def do_req(host,url,method,body=None):
	request = urllib.request.Request(host, url, {"User-Agent": "Insert immature joke here, v1.3", "Content-Type": "application/x-www-form-urlencoded"});
	do_req.cookies.add_cookie_header(request);
	do_req.con.request(method, url, body, dict(request.header_items()));
	response = do_req.con.getresponse();
	do_req.cookies.extract_cookies(response, request);
	return response.read().decode('ascii','ignore');

def schwab_login():
	global username,password;
	params = urllib.parse.urlencode({"PARAMS": "",
		"ShowUN": "Yes",
		"SANC": "",
		"SURL": "",
		"lor": "n",
		"SSNDiscFlag": "0",
		"SignonAccountNumber": username,
		"SignonPassword": password,
		"StartAnchor": "CCBodyi"});
	params = params.encode('utf-8');
	do_req("https://client.schwab.com", "/login/signon/SignOn.ashx", "POST", params);

def get_quote(sym):
	g = do_req("https://client.schwab.com",make_str("/Areas/Trade/Stocks/Quotes/EquityQuoteJson.ashx?symbol=",sym,"&isCollapsed=false&isLeg1Collapsed=false&isLeg2Collapsed=false"),"GET");

	lines = [];
	stripme = '$';
	for l in BeautifulSoup(g).prettify().splitlines():
		lines.append(l.strip().replace(',',''));

	# So, the format of their XML is awful. Basically, this is the best way to do this.
	# Take a look at the XML if you don't believe me...
	retdict = {"sym": lines[12],
		"name": lines[18].lstrip('- '),
		"last": Decimal(lines[20].strip(stripme)),
		"chg": Decimal(lines[23].strip(stripme)),
		"bid": Decimal(lines[29].strip(stripme)),
		"ask": Decimal(lines[35].strip(stripme)),
		"time:": lines[39],
		"hi:": Decimal(lines[52].strip(stripme)),
		"lo": Decimal(lines[60].strip(stripme)),
		"52hi": Decimal(lines[70].strip(stripme)),
		"52lo": Decimal(lines[78].strip(stripme)),
		"sbid": int(lines[88].strip(stripme)),
		"sask": int(lines[96].strip(stripme)),
		"vol": int(lines[106])};
	retdict['base'] = retdict['last'] - retdict['chg'];
	return retdict;

decimal.getcontext().prec = 2;

do_req.cookies = http.cookiejar.CookieJar();
do_req.con = http.client.HTTPSConnection("client.schwab.com");

do_req("https://client.schwab.com","/","GET");
schwab_login();
lquote = get_quote(sys.argv[1]);
while True:
	quote = get_quote(sys.argv[1]);
	if quote['vol'] != lquote['vol']:
		if sys.argv[2] == "-q":
			print(quote['last']);
		else:
			print(quote['vol'] - lquote['vol'],"@",quote['last']," (",quote['sbid'] * 100,"@",quote['bid'],"/",quote['sask'] * 100,"@",quote['ask'],") ",quote['chg']," %",quote['chg'] / quote['base'] * 100);
		lquote = copy.deepcopy(quote);
	time.sleep(0.1);
