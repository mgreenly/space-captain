# The executable's name
SERVER = space-captain

# The default installation directory
PREFIX ?= ${HOME}/.local/bin

# The common build flags
CFLAGS = -D_DEFAULT_SOURCE -std=c18 -pedantic -Wall -Wextra -g
LDFLAGS = -lpthread

default: build

bin/$(SERVER): $(wildcard src/*) makefile | bin
	$(CC) -o bin/$(SERVER) src/main.c $(CFLAGS) $(LDFLAGS)

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
build: bin/$(SERVER)

tests: CFLAGS := $(CFLAGS) -fsanitize=address,undefined -O0
tests: LDFLAGS := $(LDFLAGS)
tests: $(patsubst tst/%.c, bin/%, $(wildcard tst/*_tests.c))

release: CFLAGS := $(CFLAGS) -O2
release: LDFLAGS := $(LDFLAGS)
release: bin/$(SERVER)

install: $(PREFIX)
	install -m 0755 bin/$(SERVER) $(PREFIX)

clean:
	rm -rf result
	rm -rf bin
	rm -f  tags

run: bin/$(SERVER)
	./bin/$(SERVER) dat/state.dat dat/dump.dat

test: $(patsubst tst/%.c, bin/%, $(wildcard tst/*_tests.c))
	@for filename in $^; do \
		./$$filename ; \
	done

debug: bin/$(SERVER)
	@gdb -ex run -q --tui --args ./bin/$(SERVER) dat/state.dat dat/dump.dat
