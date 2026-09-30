

CC = gcc
CFLAGS = -Wall -Wextra -ggdb

all: server/server.o client/client.o
	$(CC) $(CFLAGS) server/server.o -o server_example
	$(CC) $(CFLAGS) client/client.o -o client_example

server/server.o:
	$(CC) $(CFLAGS) server/main.c -c -o server/server.o

client/client.o:
	$(CC) $(CFLAGS) client/main.c -c -o client/client.o

clean:
	rm server_example server/server.o
	rm client_example client/client.o