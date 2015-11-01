#!/usr/bin/python2

## Written by Calvin Owens <jcalvinowens@gmail.com>
## Based on: http://www.yisongyue.com/shaney/shaney_python.zip

## Just generate phrases of any length, unlike the tweet script. Optional
## integer argument will make it output that many phrases on seperate lines.

## This program is released into the public domain. This applies worldwide. In
## case this is not legally possible, the creator grants anyone the right to use
## this work for any purpose, without any conditions, unless such conditions are
## required by law.

import os;
import sys;
import random;
import string;
import re;

word_dict = {};
end_sentence = [];
file_index = [];

def choice(cur_list):
	return cur_list[random.SystemRandom().randint(0, len(cur_list) - 1)];

# I'm being lazy here, don't judge me
def delete_unmatched_parens(s):
	t = 0;
	ret = "";
	for c in s:
		if c is '(':
			t += 1;
		elif c is ')':
			if t is 0:
				continue;
			t -= 1;
		ret += c;
	while t > 0:
		n = ret.rfind('(');
		ret = ret[:n] + ret[(n + 1):];
		t -= 1;
	return ret;

def fix_string(s):
	s = re.sub('[\"]', '', s);
	s = delete_unmatched_parens(s);
	s = s[0].upper() + s[1:];
	return s;

def read_file(filename):
	global word_dict, end_sentence;

	file = open(filename, 'r');
	text = file.read();
	file.close();

	word_list = string.split(text);
	file_index.append(filename);
	cur = len(file_index) - 1;
	
	prev1 = "";
	prev2 = "";
	tmp_es = [];
	for word in word_list:
		if word in ['-','.',',']:
			continue;
		if prev1 != '' and prev2 != '':
			key = (prev2.lower(), prev1.lower());
			if key in word_dict:
				word_dict[key].append((word,cur));
			else:
				word_dict[key] = [(word,cur)];
				if prev1[-1] in ['.','?','!']:
					tmp_es.append(key);
		prev2 = prev1;
		prev1 = word;

	end_sentence.extend(tmp_es);
	return tmp_es;

def get_phrase(flist=None, bes=end_sentence,res=end_sentence):
	global word_dict, end_sentence;

	start = choice(bes);
	wl = [start[0], start[1]];

	while True:
		key = (wl[-2].lower(), wl[-1].lower());
	
		# If we hit a non-existant word tuple, recurse. This only
		# happens about 1/500 or so times, so it should be fine.
		try: word = choice(word_dict[key]);
		except KeyError: return get_phrase(flist, bes, res);

		if flist is not None:
			flist[word[1]] += 1;

		wl.append(word[0]);

		if (wl[-2], wl[-1]) in res:
			break;

	# Generate string from list of words, fix it up, and return it
	return fix_string(" ".join(wl[2:]));

def main():
	global word_dict;

	read_file("torvalds.txt");
	read_file("jesus.txt");
	
	if len(sys.argv) > 1:
		count = int(sys.argv[1])
	else:
		count = 1;
	
	for x in range(0, count):
		print(get_phrase());

main();
