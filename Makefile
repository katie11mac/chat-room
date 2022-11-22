CFLAGS=-Wall -pedantic -g 

.PHONY: all
all: chat-client chat-server

chat-client: chat-client.c
	gcc $(CFLAGS) -o $@ $^

chat-server: chat-server.c
	gcc $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f chat-client chat-server