all: server client

.PHONY: all

server: sqlite src/server/*.c src/server/*.h
	gcc src/server/*.c -o bin/server bin/*.o -I./include -lpthread -ldl

client: src/client/*.c src/client/*.h
	gcc src/client/*.c -o bin/client -I./include

sqlite: deps/sqlite/*.c
	gcc -c -o bin/sqlite.o deps/sqlite/sqlite3.c

clean: 
	- rm bin/*