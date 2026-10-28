

CC = gcc
CFLAGS = -Wall -Wextra -ggdb -fsanitize=address,undefined

all: writer/writer.o reader/reader.o
	$(CC) $(CFLAGS) writer/writer.o -o writer
	$(CC) $(CFLAGS) reader/reader.o -o reader

writer/writer.o: writer/main.c
	$(CC) $(CFLAGS) writer/main.c -I./include -c -o writer/writer.o

reader/reader.o: reader/main.c
	$(CC) $(CFLAGS) reader/main.c -I./include -c -o reader/reader.o

clean:
	rm -f writer writer/writer.o
	rm -f reader reader/reader.o