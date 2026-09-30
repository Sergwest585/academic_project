

CC = gcc
CFLAGS = -Wall -Wextra -ggdb

all: semaphore_example.o
	$(CC) $(CFLAGS) semaphore_example.o -o semaphore_example

semaphore_example.o:
	$(CC) $(CFLAGS) main.c -c -o semaphore_example.o

clean:
	rm semaphore_example semaphore_example.o