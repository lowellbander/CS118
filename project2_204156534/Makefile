CC=gcc
CFLAGS=-Wall
SRCS=client.c server.c

all: client server
	
depend: .depend

.depend: $(SRCS)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $^ > ./.depend;

include .depend

client: client.o
	g++ client.o -o client

server: server.o
	g++ server.o -o server

server.o: server.c 
	$(CC) -c server.c 

client.o: client.c
	$(CC) -c client.c

clean:
	rm -rf server client *.o
