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
	LDFLAGS =
endif

CHECKS = -fsanitize=address,undefined

bin/$(NAME): $(wildcard src/*) makefile
	mkdir -p bin
	$(CC) -o bin/$(NAME) src/main.c $(CFLAGS) $(LDFLAGS) $(CHECKS)

bin/$(NAME)-dbg: $(wildcard src/*) makefile
	mkdir -p bin
	$(CC) -o bin/$(NAME)-dbg src/main.c $(CFLAGS) -g3 -O0 $(LDFLAGS) $(CHECKS)

PHONY: all
all: bin/$(NAME) bin/$(NAME)-dbg

PHONY: debug
debug: bin/$(NAME)-dbg

PHONY: valgrind
valgrind: bin/$(NAME)-dbg
	valgrind -s --leak-check=full --show-leak-kinds=all --show-reachable=yes ./bin/$(NAME)-dbg

tags: $(wildcard src/*) makefile
	ctags -R --languages=C --c-kinds=+p --fields=+l src makefile

PHONY: gdb
gdb: bin/$(NAME)-dbg @gdb -q --tui --args ./bin/$(NAME)-dbg

PHONY: run
run: bin/$(NAME)
	@clear
	@./bin/$(NAME)

PHONY: install
install: all
	echo "Installing $(NAME) to $(PREFIX)"
	mkdir -p $(PREFIX)
	cp bin/$(NAME) $(PREFIX)
PHONY: clean
clean:
	rm -rf result
	rm -rf bin
	rm -f ctags
