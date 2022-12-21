myshell : main.o shell.o
	gcc -o myshell main.o shell.o

main.o : main.c
	gcc -c -o main.o main.c
shell.o : shell.c
	gcc -c -o shell.o shell.c
clean :
	rm *.o myshell