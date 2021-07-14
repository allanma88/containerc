objects = cstr.o cio.o log.o json.o path.o print.o config.o child.o parent.o run.o help.o pull.o main.o
ctrc : $(objects)
	cc $(objects) -o ctrc -lcjson -lcurl -lgcrypt

main.o: main.c
	cc -c -g main.c

help.o: help.c
	cc -c -g help.c 

pull.o: pull.c
	cc -c -g pull.c 

run.o: run.c
	cc -c -g run.c

parent.o: parent.c
	cc -c -g parent.c

child.o: child.c
	cc -c -g child.c

config.o: config.c
	cc -c -g config.c

print.o: print.c
	cc -c -g print.c

path.o: path.c
	cc -c -g path.c

cstr.o: cstr.c
	cc -c -g cstr.c

cio.o: cio.c
	cc -c -g cio.c
	
log.o: log.c
	cc -c -g log.c

json.o: json.c
	cc -c -g json.c

clean :
	rm -f *.o
	rm -f ctrc