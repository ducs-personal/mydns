#!/bin/sbin/python
#encoding=utf-8

test=list("todayisfridayjkkkeqeef33333333333333333344444444saffdqeffdsqfe");

map_dict={};
def count(string ,*list):
	if map_dict.has_key(string):
		map_dict[string]+=1;
	else:
		map_dict[string]=1;

for i in test:
	count(i);

print "\n".join(map_dict.items());


