CFLAGS=-Wall -pedantic -g 

my-client: my-client.c
	gcc $(CFLAGS) -o $@ $^

chat-server: chat-server.c
	gcc $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f my-client my-server