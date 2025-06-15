#
# make                     - build default target with debug symbols & instrumentation
# make run                 - runs the release target
# make debug               - runs the default target in the gdb:
#
# make tests               - build tests targets
# make runtests            - runs the test targets in the gdb
#
# make release             - build full optimized release target
# make runrelease          - build full optimized release target
# make install             - install release target
#

NAME = space-captain
LIBS =

#
# Override the default install location by setting PREFIX
#
PREFIX ?= ${HOME}/.local/bin

#
# In most cases there's no need to change anything else.
#
ifdef LIBS
	CFLAGS = -Wall -Wextra -pedantic -std=c18 $(shell pkg-config --cflags $(LIBS))
	LDFLAGS = $(shell pkg-config --libs $(LIBS))
else
	CFLAGS = -Wall -Wextra -pedantic -std=c18
	LDFLAGS = -lpthread
endif

CHECKS = -fsanitize=address,undefined

PHONY: runtests
runtests: $(patsubst tst/%.c, bin/%, $(wildcard tst/*_tests.c))
	@for filename in $^; do \
		./$$filename ; \
	done

PHONY: tests
tests: $(patsubst tst/%.c, bin/%, $(wildcard tst/*_tests.c))

bin/%_tests: tst/%_tests.c | bin
	gcc -o $@ $^

tst/%_tests.c: src/%.c src/%.h
	touch $@

bin:
	mkdir -p $@

# release
bin/$(NAME): $(wildcard src/*) makefile
	mkdir -p bin
	$(CC) -o bin/$(NAME) src/main.c $(CFLAGS) $(LDFLAGS) $(CHECKS)

# debug
bin/$(NAME)-dbg: $(wildcard src/*) makefile
	mkdir -p bin
	$(CC) -o bin/$(NAME)-dbg src/main.c $(CFLAGS) -g3 -O0 $(LDFLAGS) $(CHECKS)

PHONY: all
all: bin/$(NAME) bin/$(NAME)-dbg tests


PHONY: debug
debug: bin/$(NAME)-dbg
	@gdb -ex run -q --tui --args ./bin/$(NAME)-dbg dat/state.dat dat/dump.dat

PHONY: run
run: bin/$(NAME)
	@clear
	@./bin/$(NAME) dat/state.dat dat/dump.dat

PHONY: install
install: all | $(PREFIX)
	install -m 0755 bin/$(NAME) $(PREFIX)

$(PREFIX):
	mkdir -p $@

PHONY: clean
clean:
	rm -rf result
	rm -rf bin
	rm -f  tags

PHONY: valgrind
valgrind: bin/$(NAME)-dbg
	valgrind -s --leak-check=full --show-leak-kinds=all --show-reachable=yes ./bin/$(NAME)-dbg

tags: $(wildcard src/*) makefile
	ctags -R --languages=C --c-kinds=+p --fields=+l src makefile
