

CC = gcc
CFLAGS = -Wall -Wextra -ggdb

all: mygrep mychmod mycat myls

mygrep: grep/mygrep.o
	$(CC) $(CFLAGS) grep/mygrep.o -o mygrep
mychmod: chmod/mychmod.o
	$(CC) $(CFLAGS) chmod/mychmod.o -o mychmod
mycat: cat/mycat.o
	$(CC) $(CFLAGS) cat/mycat.o -o mycat
myls: ls/myls.o
	$(CC) $(CFLAGS) ls/myls.o -o myls

grep/mygrep.o: grep/main.c
	$(CC) $(CFLAGS) grep/main.c -c -o grep/mygrep.o

chmod/mychmod.o: chmod/main.c
	$(CC) $(CFLAGS) chmod/main.c -c -o chmod/mychmod.o

cat/mycat.o: cat/main.c
	$(CC) $(CFLAGS) cat/main.c -c -o cat/mycat.o

ls/myls.o: ls/main.c
	$(CC) $(CFLAGS) ls/main.c -c -o ls/myls.o

clean:
	rm mygrep mychmod mycat
	rm grep/mygre.o
	rm chmod/mychmod.o
	rm cat/mycat.o
	rm ls/myls.o