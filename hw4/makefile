CC = gcc
CFLAGS = -std=c99

all: search_server search_client

search_server: search_server.c trie.c vector.c
	$(CC) $(CFLAGS) -o $@ search_server.c trie.c vector.c

search_client: search_client.c
	$(CC) -o $@ search_client.c

clean:
	rm -f search_server search_client

.PHONY: all clean