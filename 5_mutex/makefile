

CC = gcc
CFLAGS = -Wall -Wextra -ggdb -fsanitize=address,undefined

all: mutex_example.o
	$(CC) $(CFLAGS) mutex_example.o -o mutex_example

mutex_example.o: main.c
	$(CC) $(CFLAGS) main.c -c -o mutex_example.o

clean:
	rm -f mutex_example mutex_example.o