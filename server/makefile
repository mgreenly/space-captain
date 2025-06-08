#
# Set the program's name and the library dependencies.
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


#
# make                     - build default target with debug symbols & instrumentation
# make debug               - runs the default target in the gdb:
#
# make tests               - build tests targets
# make test                - runs the test targets in the gdb
#
# make release             - build full optimized release target
# make run                 - runs the release target
# make install             - install release target
#



# release
bin/$(NAME): $(wildcard src/*) makefile
	mkdir -p bin
	$(CC) -o bin/$(NAME) src/main.c $(CFLAGS) $(LDFLAGS) $(CHECKS)

# debug
bin/$(NAME)-dbg: $(wildcard src/*) makefile
	mkdir -p bin
	$(CC) -o bin/$(NAME)-dbg src/main.c $(CFLAGS) -g3 -O0 $(LDFLAGS) $(CHECKS)

# tests
bin/$(NAME)-tst: $(wildcard src/*) $(wildcard tst/*) makefile
	mkdir -p bin
	$(CC) -o bin/$(NAME)-tst tst/main.c -g3 -O0

PHONY: tests
tests: bin/$(NAME)-tst


PHONY: all
all: bin/$(NAME) bin/$(NAME)-dbg bin/$(NAME)-tst

PHONY: debug
debug: bin/$(NAME)-dbg
	@gdb -ex run -q --tui --args ./bin/$(NAME)-dbg dat/state.dat dat/dump.dat

PHONY: test
test: bin/$(NAME)-tst
	@clear
	@./bin/$(NAME)-tst


PHONY: run
run: bin/$(NAME)
	@clear
	@./bin/$(NAME) dat/state.dat dat/dump.dat

PHONY: install
install: all
	echo "Installing $(NAME) to $(PREFIX)"
	mkdir -p $(PREFIX)
	cp bin/$(NAME) $(PREFIX)/

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
