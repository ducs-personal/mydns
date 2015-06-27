#!/usr/bin/python
#encoding=utf-8

import os
import sys
import string

from string_filter import pfilter

map_dict={};

def count(list):
	for string in list:
		if map_dict.has_key(string):
			map_dict[string]+=1;
		else:
			map_dict[string]=1;

def sect(list):
	newlist=[];
	for i in range(len(list)):
		newlist.extend(list[i].split())
	return newlist


def controller(path):
	fd=open(path,"a+");
	try:
		for entery in fd:
			print entery
			print  list(entery.decode('utf8'))
	finally:
		fd.close();


def print_top(list, n=0):
	for k,v in list:
		if n != 0:
			print k,v
			n-=1


def main(path):
	controller(path)
	print map_dict
	return  print_top((sorted(map_dict.items(),key=lambda x:x[1],reverse=1)),3)
	#return [print k,v for k,v in (sorted(map_dict.items(),key=lambda x:x[1],reverse=1))]

if __name__ == "__main__":
	if len(sys.argv) < 2:
		print 'Usage...';
	else:
		print main(sys.argv[1])











