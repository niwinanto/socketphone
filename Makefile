all: cli_ply ser_rec rtpsend rtprecv
CC=gcc
CFLAGS=-I

cli_ply: client.c
	$(CC) -o $@ $? -lpulse -lpulse-simple -lrt

ser_rec: server.c
	$(CC) -o $@ $? -lpulse -lpulse-simple -lrt

rtpsend: rtp_server.c
	gcc -o rtpsend -g rtp_server.c -lpulse -lpulse-simple -lrt `pkg-config --cflags ortp` `pkg-config --libs ortp` -lm

rtprecv: rtp_recv.c
	gcc -o rtprecv -g rtp_recv.c  -lpulse -lpulse-simple -lrt `pkg-config --cflags ortp` `pkg-config --libs ortp` -lm
	

clean:
	rm -f cli_ply ser_rec rtpsend rtprecv

