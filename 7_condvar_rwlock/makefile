

CC = gcc
CFLAGS = -Wall -Wextra -ggdb

all: condvar/condvar.o rwlock/rwlock.o
	$(CC) $(CFLAGS) condvar/condvar.o -o condvar_example
	$(CC) $(CFLAGS) rwlock/rwlock.o -o rwlock_example

condvar/condvar.o:
	$(CC) $(CFLAGS) condvar/main.c -c -o condvar/condvar.o

rwlock/rwlock.o:
	$(CC) $(CFLAGS) rwlock/main.c -c -o rwlock/rwlock.o

clean:
	rm condvar_example condvar/condvar.o
	rm rwlock_example rwlock/rwlock.o