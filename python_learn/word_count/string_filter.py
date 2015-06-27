#!/usr/bin/python
#encoding=utf-8

import re

temp = "想做/ 兼_职/学生_/ 的 、加,我Q：  1 5.  8 0. ！！？？  8 6 。0.  2。 3     有,惊,喜,哦"

#punctuation filter
def pfilter(string):
	tmp = string.decode("utf8")
	string=re.sub(r"[\s+.!/_,$%^*(+\"']+|[+——！，。？、~@#￥%……&*（）]+".decode("utf8"), "".decode("utf8"),tmp)
	print string

if __name__ == "__main__":
	punc_filter(temp)




