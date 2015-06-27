#encoding=utf-8


def travel_list(list):
	newlist=[]
	for entry in range(len(list)):
		print entry;
		newlist.extend(list[entry].split());
		
	return newlist

if __name__ == "__main__":
	a=["hello world ", "today is python"]
	print travel_list(a);
