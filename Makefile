all: server client

.PHONY: all

server: server.c
	gcc -o server server.c

client: src/client/*.c src/client/*.h
	gcc src/client/*.c -o client
