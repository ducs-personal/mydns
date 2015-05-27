# -*- coding:utf-8 -*-
import urllib
import urllib2
import sys
import re
import random

def gethtml(url):
	page=urllib.urlopen(url)
	html=page.read();
	return html;

def getimg(html):
   reg=r'src="(.+?.jpg)"'
   imgre=re.compile(reg);
   imglist=re.findall(imgre,html)
   x = 0
   for imgurl in imglist:
        urllib.urlretrieve(imgurl,'/home/shuai/图片/%s.jpg' % x)
        x+=1
   return imglist;



page = random.uniform(1,10)
url = 'http://www.qiushibaike.com/hot/page/' + str(page)
user_agent = 'Mozilla/4.0 (compatible; MSIE 5.5; Windows NT)'
headers = { 'User-Agent' : user_agent }
try:
    request = urllib2.Request(url,headers = headers)
    response = urllib2.urlopen(request)
    html= response.read()
except urllib2.URLError, e:
    if hasattr(e,"code"):
        print e.code
    if hasattr(e,"reason"):
        print e.reason

print "------------save-------------------";
print getimg(html);


