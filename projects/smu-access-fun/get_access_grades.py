#!/usr/bin/python3

## Access.SMU Grade Retrieval Utility
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

import sys;
import re;
import http.cookiejar;
import urllib;
from bs4 import BeautifulSoup;

def do_http_req(con,cookies,host,url,method,body):
	req = urllib.request.Request(host, url, {"User-Agent": "Holy clusterfuck of JavaScript, Batman!! v2.0", "Content-Type": "application/x-www-form-urlencoded"});
	cookies.add_cookie_header(req);
	con.request(method, url, body, dict(req.header_items()));
	res = con.getresponse();
	cookies.extract_cookies(res, req);
	return res.read().decode('ascii','ignore');

def get_nice_data(soupdoc,string):
	ret = soupdoc.find_all(id=string);
	if(len(ret) == 0):
		return "";
	else:
		if(ret[0].get_text() == '\xa0'):
			return "";
		return ret[0].get_text();

def get_nice_index(soupdoc,item,index):
	if(len(item) < index + 1):
		return "";
	else:
		if(item[index].get_text() == '\xa0'):
			return "";
		return item[index].get_text();

def get_grade_list(soupdoc):
	courses = soupdoc.find_all(id=re.compile('^CLS_LINK\$[0-9]{1,2}'));
	names = soupdoc.find_all(id=re.compile('^CLASS_TBL_VW_DESCR\$[0-9]{1,2}'));
	hours = soupdoc.find_all(id=re.compile('^STDNT_ENRL_SSV1_UNT_TAKEN\$[0-9]{1,2}'));
	grade_types = soupdoc.find_all(id=re.compile('^GRADING_BASIS\$[0-9]{1,2}'));
	grades = soupdoc.find_all(id=re.compile('^STDNT_ENRL_SSV1_CRSE_GRADE_OFF\$[0-9]{1,2}'));
	grade_points = soupdoc.find_all(id=re.compile('^STDNT_ENRL_SSV1_GRADE_POINTS\$[0-9]{1,2}'));

	num = len(courses);
	grade_list = [];
	i = 0;

	while i < num:
		courseee = {"Course": get_nice_index(soupdoc,courses,i),
		"Name": get_nice_index(soupdoc,names, i),
		"Credit Hours": get_nice_index(soupdoc,hours, i),
		"Grading": get_nice_index(soupdoc,grade_types, i),
		"Grade": get_nice_index(soupdoc,grades, i),
		"GPA Points": get_nice_index(soupdoc,grade_points, i)};
		grade_list.append(courseee);
		i += 1;

	return grade_list;

def get_gpa_attrs(soupdoc):
	# Unfortunately, several of these attributes seem to have been renamed over
	# the past few years. These searches work on the most recent semester.
	gb = {'sem_xfr_hrs': get_nice_data(soupdoc,"STATS_ENRL$9"),
		'sem_gpa': get_nice_data(soupdoc,"STATS_ENRL$14"),
		'sem_hrs_taken': get_nice_data(soupdoc,"STATS_ENRL$2"),
		'sem_hrs_passed': get_nice_data(soupdoc,"STATS_ENRL$3"),
		'sem_hrs_ip': get_nice_data(soupdoc,"STATS_ENRL$4"),
		'sem_gpa_pts': get_nice_data(soupdoc,"STATS_ENRL$12"),
		'sem_nfgpa_taken': get_nice_data(soupdoc,"STATS_ENRL$7"),
		'sem_nfgpa_passed': get_nice_data(soupdoc,"STATS_ENRL$8"),
		'sem_hrs_tgpa': get_nice_data(soupdoc,"STATS_ENRL$13"),
		'cuml_xfr_hrs': get_nice_data(soupdoc,"STATS_CUMS$9"),
		'cuml_gpa': get_nice_data(soupdoc,"STATS_CUMS$14"),
		'cuml_hrs_taken': get_nice_data(soupdoc,"STATS_CUMS$2"),
		'cuml_hrs_passed': get_nice_data(soupdoc,"STATS_CUMS$3"),
		'cuml_hrs_ip': get_nice_data(soupdoc,"STATS_CUMS$4"),
		'cuml_gpa_pts': get_nice_data(soupdoc,"STATS_CUMS$12"),
		'cuml_nfgpa_taken': get_nice_data(soupdoc,"STATS_CUMS$7"),
		'cuml_nfgpa_passed': get_nice_data(soupdoc,"STATS_CUMS$8"),
		'cuml_hrs_tgpa': get_nice_data(soupdoc,"STATS_CUMS$13"),
		'cuml_gpa': get_nice_data(soupdoc,"STATS_CUMS$14"),
		'tot_xfr_hrs': get_nice_data(soupdoc,"STDNT_CAR_TM_VW_TOT_TRNSFR$0"),
		'tot_tst_hrs': get_nice_data(soupdoc,"STDNT_CAR_TM_VW_TOT_TEST_CREDIT$0"),
		'tot_oth_hrs': get_nice_data(soupdoc,"STDNT_CAR_TM_VW_TOT_OTHER$0"),
		'tot_all_hrs': get_nice_data(soupdoc,"STDNT_CAR_TM_VW_TOT_CUMULATIVE$0")};

	return gb;

def get_access_gradepage_data(uid,password):
	cookies = http.cookiejar.CookieJar();
	con = http.client.HTTPSConnection("my.smu.edu");

	do_http_req(con,cookies,"https://my.smu.edu","/psc/ps/EMPLOYEE/HRMS/c/SA_LEARNER_SERVICES.CLASS_SEARCH.GBL","GET",None);
	g = do_http_req(con,cookies,"https://my.smu.edu","/psc/ps/EMPLOYEE/HRMS/c/SA_LEARNER_SERVICES.CLASS_SEARCH.GBL?&","GET",None);

	sicsid = re.search("[a-zA-Z0-9\/\+^\s]{43}\=", g);
	if sicsid == None:
		print("</pre><p style=\"max-width:500px\"><b>Access.SMU is currently down for maintenance</b></p><p style=\"max-width:500px\"><i>It's also possible Access.SMU changed and the script is broken - usually I'll notice it soon. If not, let me know and I'll fix it. If I feel like it. Maybe.</i></p><pre>", end='');
		sys.exit(-1);

	access_icsid = sicsid.group(0);

	login_post = urllib.parse.urlencode({"httpPort": "",
	"timezoneOffset": "360",
	"userid": uid,
	"pwd": password,
	"Submit": "Sign In"});
	login_post = login_post.encode('utf-8');

	post0 = urllib.parse.urlencode({"ICAJAX": "1",
	"ICNAVTYPEDROPDOWN": "1",
	"ICType": "Panel",
	"ICElementNum": "0",
	"ICStateNum": "1",
	"ICAction":"DERIVED_SSS_SCT_SSS_TERM_LINK",
	"ICXPos": "0",
	"ICYPos": "0",
	"ResponsetoDiffFrame": "-1",
	"TargetFrameName": "None",
	"GSrchRaUrl": "None",
	"FacetPath": "None",
	"ICFocus": "",
	"ICSaveWarningFilter": "0",
	"ICChanged": "-1",
	"ICResubmit": "0",
	"ICSID": access_icsid,
	"ICActionPrompt": "false",
	"ICFind": "",
	"ICAddCount": "",
	"DERIVED_SSTSNAV_SSTS_MAIN_GOTO$23$": "9999",
	"DERIVED_SSTSNAV_SSTS_MAIN_GOTO$97$": "9999"});
	post0 = post0.encode('utf-8');

	post1 = urllib.parse.urlencode({"ICAJAX": "1",
	"ICNAVTYPEDROPDOWN": "1",
	"ICType": "Panel",
	"ICElementNum": "0",
	"ICStateNum": "2",
	"ICAction": "DERIVED_SSS_SCL_SSS_GO_1",
	"ICXPos": "0",
	"ICYPos": "0",
	"ResponsetoDiffFrame": "-1",
	"TargetFrameName": "None",
	"ICFocus": "",
	"ICSaveWarningFilter": "0",
	"ICChanged": "-1",
	"ICResubmit": "0",
	"ICSID": access_icsid,
	"ICModalWidget": "0",
	"ICZoomGrid": "0",
	"ICZoomGridRt": "0",
	"ICModalLongClosed": "",
	"ICActionPrompt": "false",
	"ICFind": "",
	"ICAddCount": "",
	"DERIVED_SSS_SCL_SSS_MORE_ACADEMICS": "1030",
	"account": "",
	"student_firstname": "",
	"student_lastname": "",
	"consumer_email": "",
	"student_dob": "",
	"student_fullname": "",
	"DERIVED_SSS_SCL_SSS_MORE_PROFILE": "9999"});
	post1 = post1.encode('utf-8');


	do_http_req(con,cookies,"https://my.smu.edu", "/psp/ps/EMPLOYEE/HRMS/?cmd=login", "POST", login_post);
	do_http_req(con,cookies,"https://my.smu.edu","/psc/ps/EMPLOYEE/HRMS/c/SA_LEARNER_SERVICES.SSR_SSENRL_GRADE.GBL?Page=SSR_SSENRL_GRADE&Action=A&TargetFrameName=None","GET",None);
	p1 = do_http_req(con,cookies,"https://my.smu.edu", "/psc/ps/EMPLOYEE/HRMS/c/SA_LEARNER_SERVICES.SSR_SSENRL_GRADE.GBL", "POST", post0);

	# FIXME: After finals, they change the default to the current semester's grades,
	# instead of the semester choice dialog. It also changes sometimes during the
	# semester, no idea why...
	#p1 = do_http_req(con,cookies,"https://my.smu.edu", "/psc/ps/EMPLOYEE/HRMS/c/SA_LEARNER_SERVICES.SSS_STUDENT_CENTER.GBL", "POST", post1);
	
	p1s = BeautifulSoup(p1, "html5lib");
	sems = p1s.find_all(id=re.compile('^TERM_CAR\$[0-9]{1,2}'));
	
	i = 0;
	while i < len(sems):
		if sems[i].get_text() == "Spring 2014":
			semester = str(i);
			break;
		i += 1;

	if i == len(sems):
		print("</pre><p style=\"max-width:500px\"><b>Invalid data returned!!!</b> Are you sure you entered your SMU ID/password correctly when you created your link? Have you changed your password since then?</p><p style=\"max-width:500px\"><i>It's also possible Access.SMU changed and the script is broken - usually I'll notice it soon. If not, let me know and I'll fix it. If I feel like it. Maybe.</i></p><pre>", end='');
		sys.exit(-1);

	post2 = urllib.parse.urlencode({"ICAJAX": "1",
	"ICNAVTYPEDROPDOWN": "1",
	"ICType": "Panel",
	"ICElementNum": "0",
	"ICStateNum": "2",
	"ICAction": "DERIVED_SSS_SCT_SSR_PB_GO",
	"ICXPos": "0",
	"ICYPos": "0",
	"ResponsetoDiffFrame": "-1",
	"TargetFrameName": "None",
	"ICFocus": "",
	"ICSaveWarningFilter": "0",
	"ICChanged": "-1",
	"ICResubmit": "0",
	"ICSID": access_icsid,
	"ICModalWidget": "0",
	"ICZoomGrid": "0",
	"ICZoomGridRt": "0",
	"ICModalLongClosed": "",
	"ICActionPrompt": "false",
	"ICFind": "",
	"ICAddCount": "",
	"DERIVED_SSTSNAV_SSTS_MAIN_GOTO$23$": "9999",
	"SSR_DUMMY_RECV1$sels$0": semester,
	"DERIVED_SSTSNAV_SSTS_MAIN_GOTO$72$": "9999"});
	post2 = post2.encode('utf-8');

	p2 = do_http_req(con,cookies,"https://my.smu.edu", "/psc/ps/EMPLOYEE/HRMS/c/SA_LEARNER_SERVICES.SSR_SSENRL_GRADE.GBL", "POST", post2);
	return p2;

def print_pretty_long_grades(clist,with_stats = 0,stats = {}):
	print('+-----------+--------------------------------+------+------------------+-----+--------+');
	print('| {0:9} | {1:30} | {2:4} | {3:16} | {4:3} | {5:6} |'.format('Course','Name','Hrs','Grading','Grd','Points'));
	print('+-----------+--------------------------------+------+------------------+-----+--------+');
	for c in sorted(clist, key=lambda k: k['Credit Hours'], reverse=True):
		print('| {0:9} | {1:30} | {2:4} | {3:16} | {4:3} | {5:6} |'.format(c['Course'],c['Name'],c['Credit Hours'],c['Grading'],c['Grade'],c['GPA Points']));
	if(with_stats):
		print('+-----------+--------+-----------------------+------+------------------+-----+--------+');
		print('| GPA Pts:  | {0:6} |'.format(stats['sem_gpa_pts']));
		print('| GPA Hrs:  | {0:6} |'.format(stats['sem_hrs_tgpa']));
		print('| GPA:      | {0:6} |'.format(stats['sem_gpa']));
		print('+-----------+--------+');
	else:
		print('+-----------+-----+---------+----------------+------+------------------+-----+--------+');

def print_pretty_short_grades(clist):
	print('+-----------+-----+');
	print('| {0:9} | {1:3} |'.format("Course","Grd"));
	print('+-----------+-----+');
	for c in sorted(clist, key=lambda k: k['Credit Hours'], reverse=True):
		print('| {0:9} | {1:3} |'.format(c['Course'],c['Grade']));

def print_pretty_cuml_stats(gb):
#	print('+-----------+-----+-+-------+');
	print("|    Semester Statistics    |");
	print("+-----------------+---------+");
	print("| GPA Hrs Taken:  | {0:7} |".format(gb['sem_hrs_taken']));
	print("| GPA Hrs Passed: | {0:7} |".format(gb['sem_hrs_passed']));
	print("| GPA Hrs In Prg: | {0:7} |".format(gb['sem_hrs_ip']));
	print("| Oth Hrs Taken:  | {0:7} |".format(gb['sem_nfgpa_taken']));
	print("| Oth Hrs Passed: | {0:7} |".format(gb['sem_nfgpa_passed']));
	print("| Total GPA Pts:  | {0:7} |".format(gb['sem_gpa_pts']));
	print("| Total GPA Hrs:  | {0:7} |".format(gb['sem_hrs_tgpa']));
	print("| Calculated GPA: | {0:7} |".format(gb['sem_gpa']));
	print("+-----------------+---------+");
	print("|   Cumulative Statistics   |")
	print("+-----------------+---------+");
	print("| GPA Hrs Taken:  | {0:7} |".format(gb['cuml_hrs_taken']));
	print("| GPA Hrs Passed: | {0:7} |".format(gb['cuml_hrs_passed']));
	print("| GPA Hrs In Prg: | {0:7} |".format(gb['cuml_hrs_ip']));
	print("| Oth Hrs Taken:  | {0:7} |".format(gb['cuml_nfgpa_taken']));
	print("| Oth Hrs Passed: | {0:7} |".format(gb['cuml_nfgpa_passed']));
	print("| Total GPA Pts:  | {0:7} |".format(gb['cuml_gpa_pts']));
	print("| Total GPA Hrs:  | {0:7} |".format(gb['cuml_hrs_tgpa']));
	print("| Calculated GPA: | {0:7} |".format(gb['cuml_gpa']));
	print("+-----------------+---------+");
	print("| Hrs from Xfer:  | {0:7} |".format(gb['tot_xfr_hrs']));
	print("| Hrs from Tests: | {0:7} |".format(gb['tot_tst_hrs']));
	print("| Hrs from Other: | {0:7} |".format(gb['tot_oth_hrs']));
	print("| Total Hrs:      | {0:7} |".format(gb['tot_all_hrs']));
	print("+-----------------+----------+");

soupdoc = BeautifulSoup(get_access_gradepage_data(sys.argv[1],sys.argv[2]), "html5lib");

clist = get_grade_list(soupdoc);
gb = get_gpa_attrs(soupdoc);

print_pretty_long_grades(clist);
print_pretty_cuml_stats(gb);
#print_pretty_long_grades(clist,1,gb);
