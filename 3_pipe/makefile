

CC = gcc
CFLAGS = -Wall -Wextra -ggdb

all: pipe_example.o
	$(CC) $(CFLAGS) pipe_example.o -o pipe_example

pipe_example.o:
	$(CC) $(CFLAGS) main.c -c -o pipe_example.o

clean:
	rm pipe_example pipe_example.o