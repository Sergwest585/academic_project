

CC = gcc
CFLAGS = -Wall -Wextra -ggdb -fsanitize=address,undefined

all: server/server.o client/client.o
	$(CC) $(CFLAGS) server/server.o -o server_example
	$(CC) $(CFLAGS) client/client.o -o client_example

server/server.o: server/main.c
	$(CC) $(CFLAGS) server/main.c -c -o server/server.o

client/client.o: client/main.c
	$(CC) $(CFLAGS) client/main.c -c -o client/client.o

clean:
	rm -f server_example server/server.o
	rm -f client_example client/client.o