#coding=utf-8


import urllib
import urllib2

 
requset = urllib2.Request('http://www.baidu.com')
try:
    html=urllib2.urlopen(requset,timeout=10)
except urllib2.URLError, e:
    print e.reason

print html.read();
