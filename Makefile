# single_chan_pkt_fwd
# Single Channel LoRaWAN Gateway

CC=g++
CFLAGS=-c -Wall
LIBS=-lwiringPi -ljson-c

all: single_chan_pkt_fwd

single_chan_pkt_fwd: hal.o base64.o main.o
	$(CC) main.o base64.o hal.o $(LIBS) -o single_chan_pkt_fwd

main.o: main.cpp
	$(CC) $(CFLAGS) main.cpp

base64.o: base64.c
	$(CC) $(CFLAGS) base64.c

hal.o: hal.cpp
	$(CC) $(CFLAGS) hal.cpp

clean:
	rm *.o single_chan_pkt_fwd
