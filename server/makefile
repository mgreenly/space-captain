# The executable's name
BIN = sc

# Change prefix to override the default installation directory.
PREFIX ?= ${HOME}/.local/bin

# Setup the flags that are common across all build targets
CFLAGS = -Wall -Wextra -pedantic -std=c18 -g
LDFLAGS = -lpthread

default: build

PHONY: build clean debug install release run

bin/$(BIN): $(wildcard src/*) makefile | bin
	$(CC) -o bin/$(BIN) src/main.c $(CFLAGS) $(LDFLAGS)

tags: $(wildcard src/*) makefile
	ctags -R --languages=C --c-kinds=+p --fields=+l src makefile

bin:
	mkdir -p $@

$(PREFIX):
	mkdir -p $@

build: CFLAGS := $(CFLAGS) -fsanitize=address,undefined -O0
build: LDFLAGS := $(LDFLAGS)
build: bin/$(BIN)

release: CFLAGS := $(CFLAGS) -O2
release: LDFLAGS := $(LDFLAGS)
release: bin/$(BIN)

install: $(PREFIX)
	install -m 0755 bin/$(BIN) $(PREFIX)

clean:
	rm -rf result
	rm -rf bin
	rm -f  tags

run: bin/$(BIN)
	./bin/$(BIN) dat/state.dat dat/dump.dat

debug: bin/$(BIN)
	@gdb -ex run -q --tui --args ./bin/$(BIN) dat/state.dat dat/dump.dat
