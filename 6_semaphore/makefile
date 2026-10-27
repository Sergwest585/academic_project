

CC = gcc
CFLAGS = -Wall -Wextra -ggdb -fsanitize=address,undefined

all: semaphore_example.o
	$(CC) $(CFLAGS) semaphore_example.o -o semaphore_example

semaphore_example.o: main.c
	$(CC) $(CFLAGS) main.c -c -o semaphore_example.o

clean:
	rm -f semaphore_example semaphore_example.o