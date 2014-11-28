CC=gcc
CFLAGS=-Wall

all: client server
	
client: client.o
	g++ client.o -o client

server: server.o
	g++ server.o -o server

server.o: server.c
	$(CC) -c server.c 

client.o: client.c 
	$(CC) -c client.c

clean:
	rm -rf server client
