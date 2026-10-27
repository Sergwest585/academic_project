

CC = gcc
CFLAGS = -Wall -Wextra -ggdb -fsanitize=address,undefined

all: pipe/pipe_example.o fifo/fifo_example.o
	$(CC) $(CFLAGS) pipe/pipe_example.o -o pipe_example
	$(CC) $(CFLAGS) fifo/fifo_example.o -o fifo_example

pipe/pipe_example.o: pipe/main.c
	$(CC) $(CFLAGS) pipe/main.c -c -o pipe/pipe_example.o

fifo/fifo_example.o: fifo/main.c
	$(CC) $(CFLAGS) fifo/main.c -c -o fifo/fifo_example.o

clean:
	rm -f pipe_example pipe/pipe_example.o
	rm -f fifo_example fifo/fifo_example.o
