

CC = gcc
CFLAGS = -Wall -Wextra -ggdb -fsanitize=address,undefined

all: fork_example.o
	$(CC) $(CFLAGS) fork_example.o -o fork_example

fork_example.o: main.c
	$(CC) $(CFLAGS) main.c -c -o fork_example.o

clean:
	rm -f fork_example fork_example.o