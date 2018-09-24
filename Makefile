all: server client

.PHONY: all

server: server.c
	gcc -o server server.c sqliteRun.c sqlite3.c -lpthread -ldl

client: src/client/*.c src/client/*.h
	gcc src/client/*.c -o client
