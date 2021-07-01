objects = help.o config.o cdir.o cstr.o print.o cio.o log.o parser.o child.o parent.o run.o main.o
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

config.o: config.c
	cc -c -g config.c

parser.o: parser.c
	cc -c -g parser.c

print.o: print.c
	cc -c -g print.c

cdir.o: cdir.c
	cc -c -g cdir.c

cstr.o: cstr.c
	cc -c -g cstr.c

cio.o: cio.c
	cc -c -g cio.c
	
log.o: log.c
	cc -c -g log.c

clean :
	rm *.o
	rm ctrc