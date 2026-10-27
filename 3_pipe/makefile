

CC = gcc
CFLAGS = -Wall -Wextra -ggdb -fsanitize=address,undefined

all: pipe_example.o
	$(CC) $(CFLAGS) pipe_example.o -o pipe_example

pipe_example.o: main.c
	$(CC) $(CFLAGS) main.c -c -o pipe_example.o

clean:
	rm -f pipe_example pipe_example.o