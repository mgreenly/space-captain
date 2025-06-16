# The executable's name
BIN = space-captain

# The default installation directory
PREFIX ?= ${HOME}/.local/bin

# The common build flags
CFLAGS = -Wall -Wextra -pedantic -std=c18 -g -D_POSIX_C_SOURCE=201710L -D_DEFAULT_SOURCE
LDFLAGS = -lpthread

default: build

bin/$(BIN): $(wildcard src/*) makefile | bin
	$(CC) -o bin/$(BIN) src/main.c $(CFLAGS) $(LDFLAGS)

bin/%_tests: tst/%_tests.c | bin
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

tst/%_tests.c: src/%.c src/%.h
	touch $@

tags: $(wildcard src/*) makefile
	ctags -R --languages=C --c-kinds=+p --fields=+l src makefile

bin:
	mkdir -p $@

$(PREFIX):
	mkdir -p $@

PHONY: build clean debug install release run test tests

build: CFLAGS := $(CFLAGS) -fsanitize=address,undefined -O0
build: LDFLAGS := $(LDFLAGS)
build: bin/$(BIN)

tests: CFLAGS := $(CFLAGS) -fsanitize=address,undefined -O0
tests: LDFLAGS := $(LDFLAGS)
tests: $(patsubst tst/%.c, bin/%, $(wildcard tst/*_tests.c))

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

test: $(patsubst tst/%.c, bin/%, $(wildcard tst/*_tests.c))
	@for filename in $^; do \
		./$$filename ; \
	done

debug: bin/$(BIN)
	@gdb -ex run -q --tui --args ./bin/$(BIN) dat/state.dat dat/dump.dat
