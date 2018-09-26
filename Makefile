# make the client and server programs of PiRoom

# define dependecys
SERVER_SRC=paf.c paf.h server.c sqliteInterface.c sqliteInterface.h
SERVER_DEPS=queue vec sqlite3

CLIENT_SRC=client.c terman.c terman.h
CLIENT_DEPS=

# libs and dirs
LIBS=-lpthread -ldl
INCLUDE_DIR=include/
ODIR=bin/obj/

# compiler and flags
CC=gcc
CFLAGS=-I$(INCLUDE_DIR) $(LIBS)

# generate dependencys (strong are used in gcc command)
_WEAK_CLIENT_DEPS=$(call weak_deps, $(CLIENT_DEPS), $(CLIENT_SRC), src/client/)
_CLIENT_DEPS=$(call strong_deps, $(CLIENT_DEPS), $(CLIENT_SRC), src/client/)
_WEAK_SERVER_DEPS=$(call weak_deps, $(SERVER_DEPS), $(SERVER_SRC), src/server/)
_SERVER_DEPS=$(call strong_deps, $(SERVER_DEPS), $(SERVER_SRC), src/server/)

# functions to generate deps
weak_deps=$(strip $(patsubst %,$(INCLUDE_DIR)%.h,$($(1))) $(patsubst %,$(3)%,$(filter %.h,$(2))))
strong_deps=$(strip $(patsubst %,$(3)%,$(filter %.c,$(2))) $(patsubst %,$(ODIR)%.o,$(1)))

# shortcuts
.PHONY: all
all: server client
	@echo "all done"
.PHONY: server
server: bin/server
	@echo "server done"
.PHONY: client
client: bin/client
	@echo "client done"
remake: clean all
	@echo "remake done"

# clean the bin directory
.PHONY: clean
clean: 
	rm -rf bin/
	mkdir -p $(ODIR)
.PHONY: remake

# build dependencys and shared code
$(ODIR)%.o: src/shared/%.c
	$(CC) -c -o $@ $^ $(CFLAGS)
$(ODIR)%.o: deps/%.c
	$(CC) -c -o $@ $^ $(CFLAGS)

# build client and server
bin/server: $(_SERVER_DEPS) $(_WEAK_SERVER_DEPS)
	$(CC) $(_SERVER_DEPS) -o $@ $(CFLAGS)

bin/client: $(_CLIENT_DEPS) $(_WEAK_CLIENT_DEPS)
	$(CC) $(_CLIENT_DEPS) -o $@ $(CFLAGS)