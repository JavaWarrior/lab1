#choose whether to compile with gcc or g++
CC=gcc
# CC = g++
CFLAGS = -c 


tinyShell: main.o
	$(CC) main.o -o tinyShell

main: main.c err_codes.h
	$(CC) main.c -o main.o

clean: 
	rm tinyShell main.o
