CC = gcc
CFLAGS = -pthread

all: server client

server: server.c freq_analysis.c
	$(CC) $(CFLAGS) $^ -o $@

client: client.c
	$(CC) $^ -o $@

clean:
	rm -f server client
