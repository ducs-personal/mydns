#coding=utf-8

import re
import urllib

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


html=gethtml("http://www.qiushibaike.com/hot/page");
print getimg(html);


