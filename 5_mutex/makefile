

CC = gcc
CFLAGS = -Wall -Wextra -ggdb

all: mutex_example.o
	$(CC) $(CFLAGS) mutex_example.o -o mutex_example

mutex_example.o:
	$(CC) $(CFLAGS) main.c -c -o mutex_example.o

clean:
	rm mutex_example mutex_example.o