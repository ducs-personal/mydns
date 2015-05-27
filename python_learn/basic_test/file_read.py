filename="./b.html"
i=0;
f = open(filename)
for line in f:
    print line;
    i+=1;
    print i;
f.close()

