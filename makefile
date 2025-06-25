# The executable's name
SERVER = server
CLIENT = client

# The default installation directory
PREFIX ?= ${HOME}/.local/bin

# The common build flags
CFLAGS = -D_DEFAULT_SOURCE -std=c18 -pedantic -Wall -Wextra -g
LDFLAGS =

all: bin/$(SERVER) bin/$(CLIENT)

#
# All the actual file targets.
#
bin/$(SERVER): CFLAGS := $(CFLAGS) -fsanitize=address,undefined -O0
bin/$(SERVER): LDFLAGS := $(LDFLAGS) -lpthread
bin/$(SERVER): $(wildcard src/*) makefile | bin
	$(CC) -o bin/$(SERVER) src/server.c $(CFLAGS) $(LDFLAGS)

bin/$(CLIENT): CFLAGS := $(CFLAGS) -fsanitize=address,undefined -O0
bin/$(CLIENT): LDFLAGS := $(LDFLAGS)
bin/$(CLIENT): $(wildcard src/*) makefile | bin
	$(CC) -o bin/$(CLIENT) src/client.c $(CFLAGS) $(LDFLAGS)

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
build: all

PHONY: tests
tests: CFLAGS := $(CFLAGS) -fsanitize=address,undefined -O0
tests: $(patsubst tst/%.c, bin/%, $(wildcard tst/*_tests.c))

PHONY: release
release: CFLAGS := $(CFLAGS) -O2
release: bin
	@VERSION=$$(cat .VERSION); \
	BUILD=$$(cat .BUILDNUMBER); \
	TAG=$${TAG:-build}; \
	DATE=$$(date +%Y%m%d); \
	$(CC) -o bin/$(SERVER)-$$VERSION-$$TAG.$$DATE.$$BUILD src/server.c $(CFLAGS) -O2 $(LDFLAGS) -lpthread; \
	$(CC) -o bin/$(CLIENT)-$$VERSION-$$TAG.$$DATE.$$BUILD src/client.c $(CFLAGS) -O2 $(LDFLAGS); \
	ln -sf $(SERVER)-$$VERSION-$$TAG.$$DATE.$$BUILD bin/$(SERVER); \
	ln -sf $(CLIENT)-$$VERSION-$$TAG.$$DATE.$$BUILD bin/$(CLIENT); \
	$(MAKE) bump

PHONY: install
install: $(PREFIX)
	install -m 0755 bin/$(SERVER) $(PREFIX)
	install -m 0755 bin/$(CLIENT) $(PREFIX)

#
# All the task targets.
#
PHONY: clean
clean:
	rm -rf result
	rm -rf bin
	rm -f  tags

PHONY: test
test: $(patsubst tst/%.c, bin/%, $(wildcard tst/*_tests.c))
	@for filename in $^; do \
		./$$filename ; \
	done

PHONY: run-server
run-server: bin/$(SERVER)
	./bin/$(SERVER) dat/state.dat dat/dump.dat

PHONY: run-client
run-client: bin/$(CLIENT)
	./bin/$(CLIENT)

PHONY: debug-sever
debug-server: bin/$(SERVER)
	@gdb -ex run -q --tui --args ./bin/$(SERVER) dat/state.dat dat/dump.dat

PHONY: debug-client
debug-client: bin/$(CLIENT)
	@gdb -q --tui --args ./bin/$(CLIENT)

PHONY: fmt
fmt:
	@find . -path ./tst/vendor -prune -o \( -name "*.c" -o -name "*.h" \) -type f -print0 | xargs -0 -r clang-format -i

PHONY: bump
bump:
	@echo $$(($$(cat .BUILDNUMBER) + 1)) > .BUILDNUMBER
	@echo "Build number bumped to $$(cat .BUILDNUMBER)"

PHONY: bump-patch
bump-patch:
	@VERSION=$$(cat .VERSION); \
	MAJOR=$$(echo $$VERSION | cut -d. -f1); \
	MINOR=$$(echo $$VERSION | cut -d. -f2); \
	PATCH=$$(echo $$VERSION | cut -d. -f3); \
	NEW_PATCH=$$(($$PATCH + 1)); \
	echo "$$MAJOR.$$MINOR.$$NEW_PATCH" > .VERSION; \
	echo "Version bumped to $$(cat .VERSION)"

PHONY: bump-minor
bump-minor:
	@VERSION=$$(cat .VERSION); \
	MAJOR=$$(echo $$VERSION | cut -d. -f1); \
	MINOR=$$(echo $$VERSION | cut -d. -f2); \
	NEW_MINOR=$$(($$MINOR + 1)); \
	echo "$$MAJOR.$$NEW_MINOR.0" > .VERSION; \
	echo "Version bumped to $$(cat .VERSION)"

PHONY: bump-major
bump-major:
	@VERSION=$$(cat .VERSION); \
	MAJOR=$$(echo $$VERSION | cut -d. -f1); \
	NEW_MAJOR=$$(($$MAJOR + 1)); \
	echo "$$NEW_MAJOR.0.0" > .VERSION; \
	echo "Version bumped to $$(cat .VERSION)"
