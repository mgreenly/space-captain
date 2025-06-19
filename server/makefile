# The executable's name
SERVER = space-captain

# The default installation directory
PREFIX ?= ${HOME}/.local/bin

# The common build flags
CFLAGS = -D_DEFAULT_SOURCE -std=c18 -pedantic -Wall -Wextra -g
LDFLAGS = -lpthread

default: build

#
# All the actual file targets.
#
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

#
# All the phony build targets.
#
PHONY: build
build: CFLAGS := $(CFLAGS) -fsanitize=address,undefined -O0
build: LDFLAGS := $(LDFLAGS)
build: bin/$(SERVER)

PHONY: tests
tests: CFLAGS := $(CFLAGS) -fsanitize=address,undefined -O0
tests: LDFLAGS := $(LDFLAGS)
tests: $(patsubst tst/%.c, bin/%, $(wildcard tst/*_tests.c))

PHONY: release
release: CFLAGS := $(CFLAGS) -O2
release: LDFLAGS := $(LDFLAGS)
release: bin/$(SERVER)

PHONY: install
install: $(PREFIX)
	install -m 0755 bin/$(SERVER) $(PREFIX)

#
# All the task targets.
#
PHONY: clean
clean:
	rm -rf result
	rm -rf bin
	rm -f  tags

PHONY: run
run: bin/$(SERVER)
	./bin/$(SERVER) dat/state.dat dat/dump.dat

PHONY: test
test: $(patsubst tst/%.c, bin/%, $(wildcard tst/*_tests.c))
	@for filename in $^; do \
		./$$filename ; \
	done

PHONY: debug
debug: bin/$(SERVER)
	@gdb -ex run -q --tui --args ./bin/$(SERVER) dat/state.dat dat/dump.dat
