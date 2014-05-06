all: myshell

myshell: myshell.o LineParser.o history.o List.o
	gcc  -g -ansi -Wall myshell.o history.o List.o LineParser.o -lreadline -o myshell

myshell.o: myshell.c
	gcc -g  -ansi -Wall -c myshell.c

LineParser.o: LineParser.c LineParser.h
	gcc -g -ansi -Wall -c LineParser.c

history.o: history.c History.h
	gcc -g  -ansi -Wall -c history.c

List.o:	List.c List.h
	gcc -g -ansi -Wall -c List.c

clean:
	rm *.o myshell
