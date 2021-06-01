objects = help.o config.o cust_dir.o print.o parser.o child.o parent.o run.o main.o
ctrc : $(objects)
	cc $(objects) -o ctrc -lcjson

main.o: main.c
	cc -c -g main.c

help.o: help.c
	cc -c -g help.c 

run.o: run.c
	cc -c -g run.c

parent.o: parent.c
	cc -c -g parent.c

child.o: child.c
	cc -c -g child.c

cust_dir.o: cust_dir.c
	cc -c -g cust_dir.c

config.o: config.c
	cc -c -g config.c

parser.o: parser.c
	cc -c -g parser.c

print.o: print.c
	cc -c -g print.c
	
clean :
	rm *.o
	rm ctrc