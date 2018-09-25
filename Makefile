all: server client

.PHONY: all

server: queue sqlite src/server/*.c src/server/*.h
	gcc src/server/*.c bin/*.o -o bin/server -I./include -lpthread -ldl

client: src/client/*.c src/client/*.h
	gcc src/client/*.c bin/*.o -o bin/client -I./include -lpthread -ldl

sqlite: deps/sqlite3.c
	gcc -c -o bin/sqlite.o deps/sqlite3.c

queue: src/shared/queue.c
	gcc -c -o bin/queue.o src/shared/queue.c -I./include

clean: 
	- rm bin/*