

CC = gcc
CFLAGS = -Wall -Wextra -ggdb -fsanitize=address,undefined

all: condvar/condvar.o rwlock/rwlock.o
	$(CC) $(CFLAGS) condvar/condvar.o -o condvar_example
	$(CC) $(CFLAGS) rwlock/rwlock.o -o rwlock_example

condvar/condvar.o: condvar/main.c
	$(CC) $(CFLAGS) condvar/main.c -c -o condvar/condvar.o

rwlock/rwlock.o: rwlock/main.c
	$(CC) $(CFLAGS) rwlock/main.c -c -o rwlock/rwlock.o

clean:
	rm -f condvar_example condvar/condvar.o
	rm -f rwlock_example rwlock/rwlock.o