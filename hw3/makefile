CC = gcc

CFLAGS = -Wall -g

TARGETS = server client

SRCS_SERVER = server.c
SRCS_CLIENT = client.c

all: $(TARGETS)

server: $(SRCS_SERVER)
	$(CC) $(CFLAGS) -o server $(SRCS_SERVER) -lpthread

client: $(SRCS_CLIENT)
	$(CC) $(CFLAGS) -o client $(SRCS_CLIENT)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
