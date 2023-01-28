CC = gcc
CFLAGS = -std=gnu99 -Werror -Wall -g -pthread

all: a2 

fork: a2.c
	$(CC) $(CFLAGS) -o a2 a2.c

clean:
	rm -rf a2

