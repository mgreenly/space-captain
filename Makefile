# Space Captain Makefile
# Compiler
CC = gcc

# Shared flags for all builds
CFLAGS_COMMON = -D_DEFAULT_SOURCE -std=c18 -pedantic -Wall -Wextra -g -MMD -MP

# Debug-specific flags
CFLAGS_DEBUG = $(CFLAGS_COMMON) -O0

# Release-specific flags
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O3

# Default to debug flags
CFLAGS = $(CFLAGS_DEBUG)

# Linker flags
LDFLAGS =

# Build tag (defaults to 'dev' if not specified)
TAG ?= dev

# Directories
SRC_DIR = src
TST_DIR = tst
OBJ_DIR = obj
BIN_DIR = bin
DAT_DIR = dat
PREFIX ?= $(HOME)/.local

# Main targets
TARGETS = server client

# Source files for each target
SERVER_MAIN = $(SRC_DIR)/server.c
CLIENT_MAIN = $(SRC_DIR)/client.c

# Test source files
TEST_SRCS = $(wildcard $(TST_DIR)/*_tests.c)
TEST_BINS = $(patsubst $(TST_DIR)/%.c,$(BIN_DIR)/%,$(TEST_SRCS))

# Object files - separate for debug and release
SERVER_OBJ_DEBUG = $(OBJ_DIR)/debug/server.o
CLIENT_OBJ_DEBUG = $(OBJ_DIR)/debug/client.o
SERVER_OBJ_RELEASE = $(OBJ_DIR)/release/server.o
CLIENT_OBJ_RELEASE = $(OBJ_DIR)/release/client.o

# Test object files
TEST_OBJS = $(patsubst $(TST_DIR)/%.c,$(OBJ_DIR)/%.o,$(TEST_SRCS))

# Dependency files
DEPS = $(SERVER_OBJ_DEBUG:.o=.d) $(CLIENT_OBJ_DEBUG:.o=.d) \
       $(SERVER_OBJ_RELEASE:.o=.d) $(CLIENT_OBJ_RELEASE:.o=.d) \
       $(TEST_OBJS:.o=.d)

# Default target
.PHONY: all
all: server client

# Help target
.PHONY: help
help:
	@echo "Space Captain Makefile Targets:"
	@echo ""
	@echo "Building (Debug):"
	@echo "  make                   - Build debug versions (default)"
	@echo "  make all               - Same as 'make'"
	@echo "  make server            - Build debug server"
	@echo "  make client            - Build debug client"
	@echo ""
	@echo "Building (Release):"
	@echo "  make release           - Build all release versions"
	@echo ""
	@echo "Other:"
	@echo "  make clean             - Remove build artifacts"
	@echo "  make install           - Install release versions to \$$PREFIX (default: \$$HOME/.local/bin)"
	@echo "  make run-server        - Build and run debug server"
	@echo "  make run-client        - Build and run debug client"
	@echo "  make debug-server      - Debug server with GDB"
	@echo "  make debug-client      - Debug client with GDB"
	@echo "  make fmt               - Format all *.c/*.h files with clang-format"
	@echo "  make tests             - Build all test executables"
	@echo "  make run-tests         - Build and run all tests"
	@echo "  make help              - Show this help message"
	@echo ""
	@echo "Version Management:"
	@echo "  make version           - Display version string (VERSION-TAG.BUILD)"
	@echo "  make bump-patch        - Increment patch version (0.0.X)"
	@echo "  make bump-minor        - Increment minor version (0.X.0)"
	@echo "  make bump-major        - Increment major version (X.0.0)"
	@echo "  make bump-build        - Increment build number"

# Debug targets
.PHONY: server
server: $(BIN_DIR)/server

.PHONY: client
client: $(BIN_DIR)/client

# Release targets
.PHONY: release
release:
	@$(MAKE) clean
	@$(MAKE) $(BIN_DIR)/server-release $(BIN_DIR)/client-release
	@$(MAKE) bump-build

# Debug executables
$(BIN_DIR)/server: $(SERVER_OBJ_DEBUG) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^

$(BIN_DIR)/client: $(CLIENT_OBJ_DEBUG) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^

# Release executables
$(BIN_DIR)/server-release: $(SERVER_OBJ_RELEASE) | $(BIN_DIR)
	@VERSION=$$(cat .VERSION); \
	BUILD=$$(cat .BUILD); \
	FULL_VERSION="$$VERSION-$(TAG).$$BUILD"; \
	$(CC) $(LDFLAGS) -o $(BIN_DIR)/server-$$FULL_VERSION $^; \
	ln -sf server-$$FULL_VERSION $@

$(BIN_DIR)/client-release: $(CLIENT_OBJ_RELEASE) | $(BIN_DIR)
	@VERSION=$$(cat .VERSION); \
	BUILD=$$(cat .BUILD); \
	FULL_VERSION="$$VERSION-$(TAG).$$BUILD"; \
	$(CC) $(LDFLAGS) -o $(BIN_DIR)/client-$$FULL_VERSION $^; \
	ln -sf client-$$FULL_VERSION $@

# Debug object files
$(OBJ_DIR)/debug/server.o: $(SERVER_MAIN) | $(OBJ_DIR)/debug
	$(CC) $(CFLAGS_DEBUG) -c -o $@ $<

$(OBJ_DIR)/debug/client.o: $(CLIENT_MAIN) | $(OBJ_DIR)/debug
	$(CC) $(CFLAGS_DEBUG) -c -o $@ $<

# Release object files
$(OBJ_DIR)/release/server.o: $(SERVER_MAIN) | $(OBJ_DIR)/release
	$(CC) $(CFLAGS_RELEASE) -c -o $@ $<

$(OBJ_DIR)/release/client.o: $(CLIENT_MAIN) | $(OBJ_DIR)/release
	$(CC) $(CFLAGS_RELEASE) -c -o $@ $<

# Test targets
.PHONY: tests
tests: $(TEST_BINS)

.PHONY: run-tests
run-tests: tests
	@for test in $(TEST_BINS); do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done

# Test executables
$(BIN_DIR)/%_tests: $(OBJ_DIR)/%_tests.o | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^

# Test object files
$(OBJ_DIR)/%_tests.o: $(TST_DIR)/%_tests.c | $(OBJ_DIR)
	$(CC) $(CFLAGS_DEBUG) -I$(SRC_DIR) -c -o $@ $<

# Create directories
$(BIN_DIR) $(DAT_DIR) $(OBJ_DIR)/debug $(OBJ_DIR)/release $(OBJ_DIR):
	mkdir -p $@

# Format code
.PHONY: fmt
fmt:
	find $(SRC_DIR) -name "*.c" -o -name "*.h" | xargs clang-format -i

# Clean
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Install
.PHONY: install
install: release
	install -d $(PREFIX)/bin
	install -m 755 $(BIN_DIR)/server-release $(PREFIX)/bin/space-captain-server
	install -m 755 $(BIN_DIR)/client-release $(PREFIX)/bin/space-captain-client

# Run targets
.PHONY: run-server
run-server: server | $(DAT_DIR)
	$(BIN_DIR)/server

.PHONY: run-client
run-client: client
	$(BIN_DIR)/client

# Debug targets
.PHONY: debug-server
debug-server: server
	gdb $(BIN_DIR)/server

.PHONY: debug-client
debug-client: client
	gdb $(BIN_DIR)/client

# Version management
.PHONY: version
version:
	@VERSION=$$(cat .VERSION); \
	BUILD=$$(cat .BUILD); \
	echo "$$VERSION-$(TAG).$$BUILD"

.PHONY: bump-patch
bump-patch:
	@VERSION=$$(cat .VERSION); \
	MAJOR=$$(echo $$VERSION | cut -d. -f1); \
	MINOR=$$(echo $$VERSION | cut -d. -f2); \
	PATCH=$$(echo $$VERSION | cut -d. -f3); \
	NEW_PATCH=$$(($$PATCH + 1)); \
	echo "$$MAJOR.$$MINOR.$$NEW_PATCH" > .VERSION; \
	echo "Version bumped to $$(cat .VERSION)"

.PHONY: bump-minor
bump-minor:
	@VERSION=$$(cat .VERSION); \
	MAJOR=$$(echo $$VERSION | cut -d. -f1); \
	MINOR=$$(echo $$VERSION | cut -d. -f2); \
	NEW_MINOR=$$(($$MINOR + 1)); \
	echo "$$MAJOR.$$NEW_MINOR.0" > .VERSION; \
	echo "Version bumped to $$(cat .VERSION)"

.PHONY: bump-major
bump-major:
	@VERSION=$$(cat .VERSION); \
	MAJOR=$$(echo $$VERSION | cut -d. -f1); \
	NEW_MAJOR=$$(($$MAJOR + 1)); \
	echo "$$NEW_MAJOR.0.0" > .VERSION; \
	echo "Version bumped to $$(cat .VERSION)"

.PHONY: bump-build
bump-build:
	@BUILD=$$(cat .BUILD); \
	NEW_BUILD=$$(($$BUILD + 1)); \
	echo "$$NEW_BUILD" > .BUILD; \
	echo "Build number bumped to $$(cat .BUILD)"

# Include dependency files
-include $(DEPS)
