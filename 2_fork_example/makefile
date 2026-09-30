

CC = gcc
CFLAGS = -Wall -Wextra -ggdb

all: fork_example.o
	$(CC) $(CFLAGS) fork_example.o -o fork_example

fork_example.o:
	$(CC) $(CFLAGS) main.c -c -o fork_example.o

clean:
	rm fork_example fork_example.o