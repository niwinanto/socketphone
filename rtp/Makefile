all: cli_ply ser_rec rtpsend rtprecv
CC=gcc
CFLAGS=-I

rtpsend: rtp_send.c
	gcc -o rtpsend -g rtp_send.c -lpulse -lpulse-simple -lrt `pkg-config --cflags ortp` `pkg-config --libs ortp` -lm

rtprecv: rtp_recv.c
	gcc -o rtprecv -g rtp_recv.c  -lpulse -lpulse-simple -lrt `pkg-config --cflags ortp` `pkg-config --libs ortp` -lm
	

clean:
	rm -f rtpsend rtprecv

