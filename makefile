#makefile for tinysh

tinysh: command.o tinysh.o
	gcc command.o tinysh.o -o tinysh

command.o: command.c command.h
	gcc -c command.c

tinysh.o: tinysh.c
	gcc -c tinysh.c

clean:
	rm *.o
