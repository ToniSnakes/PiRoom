# make the client and server programs of PiRoom

DEBUG=true

# dirs
INCLUDE_DIR=include/
ODIR=bin/obj/

# define dependecys
SERVER_SRC=paf.c paf.h server.c server.h storage.c storage.h
SERVER_DEPS=queue vec sqlite3
SERVER_EXTRA_DEPS=$(include)initDB.h
SERVER_LIBS=-lpthread -ldl -lcrypto

CLIENT_SRC=client.c terman.c terman.h
CLIENT_DEPS=
CLIENT_EXTRA_DEPS=
CLIENT_LIBS=


# compiler and flags
CC=gcc
CFLAGS=-I$(INCLUDE_DIR) $(if $(DEBUG), -g)

# generate dependencys (strong are used in gcc command)
_WEAK_CLIENT_DEPS=$(call weak_deps, $(CLIENT_DEPS), $(CLIENT_SRC), src/client/) $(CLIENT_EXTRA_DEPS)
_CLIENT_DEPS=$(call strong_deps, $(CLIENT_DEPS), $(CLIENT_SRC), src/client/)
_WEAK_SERVER_DEPS=$(call weak_deps, $(SERVER_DEPS), $(SERVER_SRC), src/server/) $(SERVER_EXTRA_DEPS)
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
.PHONY: remake
remake: clean all
	@echo "remake done"

# clean the bin directory
.PHONY: clean
clean: 
	rm -rf bin/
	mkdir -p $(ODIR)

# build dependencys and shared code
$(ODIR)%.o: src/shared/%.c
	$(CC) -c -o $@ $^ $(CFLAGS)
$(ODIR)%.o: deps/%.c
	$(CC) -c -o $@ $^ $(CFLAGS)

# this copys the db initialisation in initDB.sql into a string in include/initDB.h
$(include)initDB.h: src/database/initDB.sql
	echo "const char initDB[] =" > include/initDB.h
	cat src/database/initDB.sql | sed -e 's/\\/\\\\/g;s/"/\\"/g;s/\(.*\)/"\1\\n"/' >> include/initDB.h
	echo ";" >> include/initDB.h

# build client and server
bin/server: $(_SERVER_DEPS) $(_WEAK_SERVER_DEPS)
	$(CC) $(_SERVER_DEPS) -o $@ $(CFLAGS) $(SERVER_LIBS)

bin/client: $(_CLIENT_DEPS) $(_WEAK_CLIENT_DEPS)
	$(CC) $(_CLIENT_DEPS) -o $@ $(CFLAGS) $(CLIENT_LIBS)