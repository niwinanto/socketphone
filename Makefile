all: cli_ply ser_rec
CC=gcc
CFLAGS=-I

cli_ply: client.c
	$(CC) -o $@ $? -lpulse -lpulse-simple

ser_rec: server.c
	$(CC) -o $@ $? -lpulse -lpulse-simple

clean:
	rm -f cli_ply ser_rec

