CC=gcc
CFLAGS=-I.
DEPS = # header file 
OBJ = webserver.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

serverFork: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)


