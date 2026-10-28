

CC = gcc
CFLAGS = -Wall -Wextra -ggdb -fsanitize=address,undefined

all: threads/semaphore_th_example.o programs/writer/writer_example.o programs/reader/reader_example.o
	$(CC) $(CFLAGS) threads/semaphore_th_example.o -o semaphore_th_example
	$(CC) $(CFLAGS) programs/writer/writer_example.o -o writer_example
	$(CC) $(CFLAGS) programs/reader/reader_example.o -o reader_example

threads/semaphore_th_example.o: threads/main.c
	$(CC) $(CFLAGS) threads/main.c -c -o threads/semaphore_th_example.o

programs/writer/writer_example.o: programs/writer/main.c
	$(CC) $(CFLAGS) programs/writer/main.c -c -o programs/writer/writer_example.o

programs/reader/reader_example.o: programs/reader/main.c
	$(CC) $(CFLAGS) programs/reader/main.c -c -o programs/reader/reader_example.o

clean:
	rm -f semaphore_th_example threads/semaphore_th_example.o
	rm -f writer_example programs/writer/writer_example.o
	rm -f reader_example programs/reader/reader_example.o