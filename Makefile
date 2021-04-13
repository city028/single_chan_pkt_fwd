# single_chan_pkt_fwd
# Single Channel LoRaWAN Gateway

CC=g++
CFLAGS=-c -Wall
LIBS=-lwiringPi -ljson-c

all: single_chan_pkt_fwd

single_chan_pkt_fwd: udp.o hal.o base64.o gateway.o main.o
	$(CC) main.o base64.o hal.o udp.o gateway.o $(LIBS) -o single_chan_pkt_fwd

main.o: main.cpp
	$(CC) $(CFLAGS) main.c

base64.o: base64.c
	$(CC) $(CFLAGS) base64.c

hal.o: hal.cpp
	$(CC) $(CFLAGS) hal.c

udp.o: udp.c
		$(CC) $(CFLAGS) udp.c

		gateway.o: gateway.c
				$(CC) $(CFLAGS) gateway.c
clean:
	rm *.o single_chan_pkt_fwd
