

CC = gcc
CFLAGS = -Wall -Wextra -ggdb

all: grep/mygrep.o chmod/mychmod.o cat/mycat.o
	$(CC) $(CFLAGS) grep/mygrep.o -o mygrep
	$(CC) $(CFLAGS) chmod/mychmod.o -o mychmod
	$(CC) $(CFLAGS) cat/mycat.o -o mycat
	$(CC) $(CFLAGS) ls/myls.o -o myls

grep/mygrep.o:
	$(CC) $(CFLAGS) grep/main.c -c -o grep/mygrep.o

chmod/mychmod.o:
	$(CC) $(CFLAGS) chmod/main.c -c -o chmod/mychmod.o

cat/mycat.o:
	$(CC) $(CFLAGS) cat/main.c -c -o cat/mycat.o

ls/myls.o:
	$(CC) $(CFLAGS) ls/main.c -c -o ls/myls.o

clean:
	rm mygrep mychmod mycat
	rm grep/mygre.o
	rm chmod/mychmod.o
	rm cat/mycat.o
	rm ls/myls.o