#!/usr/bin/python3 -u

## SMU Password Reset Script
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
##
## Like this? Buy me a beer: 1FTV8WCDv4JzH7r1qANAr5g4Pb9ZaH1nUM (Bitcoin)

import re;
import http.cookiejar;
import urllib;
import random;
import string;
from getpass import getpass;

def do_http_req(con,cookies,host,url,method,body):
	req = urllib.request.Request(host, url, {"User-Agent": "trololololololol v1.3",
						"Content-Type": "application/x-www-form-urlencoded"});
	cookies.add_cookie_header(req);
	con.request(method, url, body, dict(req.header_items()));
	res = con.getresponse();
	cookies.extract_cookies(res, req);
	return res.read().decode('ascii','ignore');

# Yes, you actually do have to reconnect each time...
def change_password(sid,old,new):
	cookies = http.cookiejar.CookieJar();
	con = http.client.HTTPSConnection("pwreset.smu.edu");
	g = do_http_req(con,cookies,"https://pwreset.smu.edu","/apr.dll?cmd=change","GET",None);

	formdata_bullshit = re.search("[a-zA-Z0-9\/\+^\s]{43}\=", g).group(0);
	next_post = urllib.parse.urlencode({	"username": sid,
						"domain": "SMU",
						"formdata": formdata_bullshit}).encode('utf-8');
	h = do_http_req(con,cookies,"https://pwreset.smu.edu","/apr.dll?cmd=change", "POST", next_post);

	formdata_bullshit = re.search("[a-zA-Z0-9\/\+^\s]{43}\=", h).group(0);
	change_post = urllib.parse.urlencode({	"username": sid,
						"domain": "SMU",
						"oldpassword": old,
						"password": new,
						"confirmpassword": new,
						"formdata": formdata_bullshit}).encode('utf-8');
	i = do_http_req(con,cookies,"https://pwreset.smu.edu","/apr.dll?cmd=change", "POST", change_post);

	success = re.search("You can now logon with your new password", i);
	if success is not None: return True;
	else: return False;

def rotate_to_current_pw(sid,password):
	pwplus = "";
	for i in range(0,10):
		pwplus += random.choice(string.ascii_letters);
		print("PW change #" + str(i+1) + ": Appending \"" + pwplus + "\"... ",end='');
		if not change_password(sid,password + pwplus[:-1],password + pwplus):
			if i is 0:
				print("\nInitial password reset failed - did you mistype your password?");
				exit();
			print("\nPassword reset FAILED, last valid PW was:",password + pwplus[:-1]);
			exit();
		print("done!");

	print("Resetting PW to original... ",end='');
	if not change_password(sid,password + pwplus,password):
		print("\nPassword reset FAILED, last valid PW was:",password + pwplus);
		exit();
	print("Success!");

## The system SMU uses does not allow you to set a password that is one of the
## last 10 passwords you have used. This script automatically sets 10 different
## passwords in a row, and then restores your original.

sid = input("Enter your SMU ID: ");
pwd = getpass("Enter your current SMU Password: ");

rotate_to_current_pw(sid,pwd);