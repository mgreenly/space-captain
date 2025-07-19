# ============================================================================
# Space Captain Makefile
# ============================================================================
# Traditional build system - each source file compiles to its own object file
#
# Organization:
#   1. Directory Structure      - Source, build, and output directories
#   2. Version and Build        - Version info, build tags, dependencies
#   3. Installation Settings    - Install prefix and paths
#   4. Compiler Settings        - Compiler flags and options
#   5. Package Metadata         - Package information for releases
#   6. OS Detection             - Operating system detection and configuration
#   7. Architecture Detection   - Architecture mapping for packages
#   8. Build Helpers            - Helper functions and macros
#   9. File Definitions         - Source and object file lists
#  10. Build Targets            - Main build rules
#  11. Docker Configuration     - Docker settings and image definitions

# ============================================================================
# Directory Structure
# ============================================================================
SRC_DIR = src
TST_DIR = tests
OBJ_DIR = obj
BIN_DIR = bin
DAT_DIR = data

# Dependency directories
DEPS_DIR = deps
DEPS_SRC_DIR = $(DEPS_DIR)/src
DEPS_BUILD_DIR = $(DEPS_DIR)/build

# ============================================================================
# Version and Build Settings
# ============================================================================
# Function to construct version string with optional pre-release and git metadata
define get-version
	MAJOR=$$(cat .vMAJOR); \
	MINOR=$$(cat .vMINOR); \
	PATCH=$$(cat .vPATCH); \
	PRE=$$(cat .vPRE 2>/dev/null || true); \
	if [ -n "$$HOST_GIT_SHA" ]; then \
		GIT_SHA="$$HOST_GIT_SHA"; \
	else \
		GIT_SHA=$$(git rev-parse --short HEAD 2>/dev/null || echo "nogit"); \
	fi; \
	VERSION="$$MAJOR.$$MINOR.$$PATCH"; \
	if [ -n "$$PRE" ]; then VERSION="$$VERSION-pre$$PRE"; fi; \
	VERSION="$$VERSION+$$GIT_SHA"; \
	echo "$$VERSION"
endef

PACKAGE_VERSION := $(shell $(get-version))
# Package-safe version with '-' replaced by '~' for deb/rpm filenames
PACKAGE_VERSION_SAFE := $(shell $(get-version) | sed 's/-/~/g')
MBEDTLS_VERSION = v2.28.3
UNITY_VERSION = v2.5.2

# ============================================================================
# Installation Settings
# ============================================================================
PREFIX ?= /usr/local

# ============================================================================
# Compiler Settings
# ============================================================================
CC ?= gcc

# Detect compiler type
COMPILER_TYPE := unknown
ifeq ($(CC),gcc)
    COMPILER_TYPE := gcc
else ifeq ($(CC),g++)
    COMPILER_TYPE := gcc
else ifeq ($(CC),cc)
    # cc is usually a symlink to gcc or clang, detect the actual compiler
    COMPILER_VERSION := $(shell $(CC) --version 2>/dev/null | head -1)
    ifneq (,$(findstring Debian,$(COMPILER_VERSION)))
        # Debian's cc is gcc
        COMPILER_TYPE := gcc
    else ifneq (,$(findstring gcc,$(COMPILER_VERSION)))
        COMPILER_TYPE := gcc
    else ifneq (,$(findstring GCC,$(COMPILER_VERSION)))
        COMPILER_TYPE := gcc
    else ifneq (,$(findstring clang,$(COMPILER_VERSION)))
        COMPILER_TYPE := clang
    endif
else ifeq ($(CC),clang)
    COMPILER_TYPE := clang
else ifeq ($(CC),clang++)
    COMPILER_TYPE := clang
else
    # Try to detect based on version output
    COMPILER_VERSION := $(shell $(CC) --version 2>/dev/null | head -1)
    ifneq (,$(findstring gcc,$(COMPILER_VERSION)))
        COMPILER_TYPE := gcc
    else ifneq (,$(findstring GCC,$(COMPILER_VERSION)))
        COMPILER_TYPE := gcc
    else ifneq (,$(findstring clang,$(COMPILER_VERSION)))
        COMPILER_TYPE := clang
    endif
endif

# Compiler flags explanation:
# -D_DEFAULT_SOURCE           Enable default set of feature test macros
# -D_FORTIFY_SOURCE=2         Runtime buffer overflow detection
# -std=c18                    Use C18 standard
# -pedantic                   Strict ISO C compliance
# -fanalyzer                  Enable static analysis
# -Wshadow                    Warn when a local shadows another variable
# -Wstrict-prototypes         Warn about functions without prototypes
# -Wmissing-prototypes        Warn about missing prototypes
# -Wwrite-strings             Warn about deprecated writable string literals
# -Werror                     Treat warnings as errors
# -Wall                       Enable most warning messages
# -Wextra                     Enable extra warning messages
# -Wformat=2                  Enhanced format string checking
# -Wconversion                Warn about type conversions that may alter values
# -Wcast-qual                 Warn about casts that remove qualifiers
# -Wundef                     Warn about undefined macros in #if
# -g                          Generate debug information
# -MMD                        Generate dependency files
# -MP                         Add phony targets for headers
# -fstack-protector-strong    Stack smashing protection
# -fPIE                       Position independent code for ASLR
# -fstack-clash-protection    Prevent stack clash attacks
# -Wimplicit-fallthrough      Catch missing break statements
# -Walloca                    Warn about alloca usage
# -Wvla                       Warn about variable length arrays
# -Wformat-signedness         Format string sign mismatches
# -Wduplicated-cond           Duplicated conditions in if-else chains
# -Wduplicated-branches       Identical branches in if-else
# -Wnull-dereference          Potential null pointer dereferences
# -Wdouble-promotion          Float to double promotions
# -fdiagnostics-color         Enable colored compiler output
# -I$(DEPS_BUILD_DIR)         Include path for mbedTLS headers

# Common flags for all compilers
CFLAGS_BASE = -D_DEFAULT_SOURCE -D_FORTIFY_SOURCE=2 -std=c18 -pedantic \
              -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wwrite-strings \
              -Werror -Wall -Wextra -Wformat=2 -Wconversion -Wcast-qual -Wundef \
              -g -MMD -MP -fstack-protector-strong -fPIE \
              -Wimplicit-fallthrough -Walloca -Wvla \
              -Wnull-dereference -Wdouble-promotion \
              -I$(DEPS_BUILD_DIR)/$(OS_DIR)/include

# GCC-specific flags
CFLAGS_GCC = -fanalyzer -fstack-clash-protection \
             -Wduplicated-cond -Wduplicated-branches \
             -Wformat-signedness \
             -fdiagnostics-color

# Clang-specific flags
CFLAGS_CLANG = -fcolor-diagnostics \
               -Wno-gnu-zero-variadic-macro-arguments

# Select appropriate compiler-specific flags
ifeq ($(COMPILER_TYPE),gcc)
    CFLAGS_COMPILER = $(CFLAGS_GCC)
else ifeq ($(COMPILER_TYPE),clang)
    CFLAGS_COMPILER = $(CFLAGS_CLANG)
else
    # Unknown compiler - use no specific flags
    CFLAGS_COMPILER =
endif

# Combined common flags
CFLAGS_COMMON = $(CFLAGS_BASE) $(CFLAGS_COMPILER)

# Debug-specific flags:
# -Og                      Optimization for debugging (enables FORTIFY_SOURCE)
# -fsanitize=address       Enable AddressSanitizer for memory error detection
# -fsanitize=undefined     Enable UndefinedBehaviorSanitizer
# -fno-omit-frame-pointer  Better stack traces in sanitizers
CFLAGS_DEBUG = $(CFLAGS_COMMON) -Og -fsanitize=address,undefined -fno-omit-frame-pointer

# Release-specific flags:
# -O2                      Security-recommended optimization level
# -DNDEBUG                 Define NDEBUG to disable assertions
# -flto                    Enable Link Time Optimization
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O2 -DNDEBUG -flto

# ThreadSanitizer flags:
# -O1                   Basic optimization (TSAN works better with optimization)
# -fsanitize=thread     Enable ThreadSanitizer for race detection
CFLAGS_TSAN = $(CFLAGS_COMMON) -O1 -fsanitize=thread

CFLAGS = $(CFLAGS_DEBUG)  # Default to debug

# Linker flags explanation:
# -L$(DEPS_BUILD_DIR)   Library search path for mbedTLS
# -Wl,-rpath,...        Set runtime library search path
# -pie                  Create position independent executable
# -Wl,-z,relro          Read-only relocations (partial RELRO)
# -Wl,-z,now            Resolve all symbols at startup (full RELRO)
# -Wl,-z,noexecstack    Mark stack as non-executable
# -Wl,--as-needed       Only link libraries that are actually used
# -lpthread             Link pthread library
# -lmbedtls             Link mbedTLS library
# -lmbedx509            Link mbedTLS X.509 library
# -lmbedcrypto          Link mbedTLS crypto library
LDFLAGS_COMMON = -L$(DEPS_BUILD_DIR)/$(OS_DIR)/lib -Wl,-rpath,$(PWD)/$(DEPS_BUILD_DIR)/$(OS_DIR)/lib -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,--as-needed -lpthread -lmbedtls -lmbedx509 -lmbedcrypto

# Debug-specific linker flags:
# -fsanitize=address,undefined    Link AddressSanitizer and UBSan runtime
LDFLAGS_DEBUG = $(LDFLAGS_COMMON) -fsanitize=address,undefined

# Release-specific linker flags:
# -flto                 Enable Link Time Optimization
LDFLAGS_RELEASE = $(LDFLAGS_COMMON) -flto

# Compiler-specific LDFLAGS adjustments
ifeq ($(COMPILER_TYPE),gcc)
    # GCC-specific linker flags (if any)
    LDFLAGS_GCC =
else ifeq ($(COMPILER_TYPE),clang)
    # Clang-specific linker flags (if any)
    LDFLAGS_CLANG =
endif

# ThreadSanitizer linker flags:
# -fsanitize=thread     Link ThreadSanitizer runtime
LDFLAGS_TSAN = $(LDFLAGS_COMMON) -fsanitize=thread

LDFLAGS = $(LDFLAGS_DEBUG)  # Default to debug

# ============================================================================
# Package Metadata
# ============================================================================
PACKAGE_NAME = space-captain-server
PACKAGE_DESCRIPTION = Space Captain MMO Server
PACKAGE_VENDOR = Space Captain Team
PACKAGE_LICENSE = MIT
PACKAGE_URL = https://github.com/mgreenly/space-captain
PACKAGE_MAINTAINER = mgreenly@gmail.com
PACKAGE_OUT_DIR = pkg/$(OS_DIR)/out

# ============================================================================
# OS Detection and Configuration
# ============================================================================
# Detect operating system
OS_RELEASE_FILE = /etc/os-release
ifneq ($(wildcard $(OS_RELEASE_FILE)),)
    OS_ID := $(shell grep '^ID=' $(OS_RELEASE_FILE) | cut -d= -f2 | tr -d '"')
    OS_VERSION_ID := $(shell grep '^VERSION_ID=' $(OS_RELEASE_FILE) | cut -d= -f2 | tr -d '"')
else
    OS_ID := unknown
endif

# Store host OS for comparison
HOST_OS_ID := $(OS_ID)

# OS-specific settings
ifeq ($(OS_ID),debian)
    OS_DIR := debian
    PACKAGE_TYPE := deb
    PACKAGE_BUILDER := dpkg-deb
else ifeq ($(OS_ID),ubuntu)
    OS_DIR := ubuntu
    PACKAGE_TYPE := deb
    PACKAGE_BUILDER := dpkg-deb
else ifeq ($(OS_ID),amzn)
    OS_DIR := amazon
    PACKAGE_TYPE := rpm
    PACKAGE_BUILDER := rpmbuild
else ifeq ($(OS_ID),fedora)
    OS_DIR := fedora
    PACKAGE_TYPE := rpm
    PACKAGE_BUILDER := rpmbuild
else
    $(error Unsupported OS: $(OS_ID). This project only supports Debian (ID=debian), Ubuntu (ID=ubuntu), Amazon Linux (ID=amzn) and Fedora (ID=fedora))
endif

# Check if we're building for the host OS
ifeq ($(HOST_OS_ID),$(OS_ID))
    IS_HOST_BUILD := 1
else
    IS_HOST_BUILD := 0
endif

# Update BIN_DIR to be OS-specific
BIN_DIR_OS = $(BIN_DIR)/$(OS_DIR)

# ============================================================================
# Architecture Detection
# ============================================================================
# Get raw machine architecture
MACHINE_ARCH := $(shell uname -m)

# Architecture mappings for different package formats
ifeq ($(MACHINE_ARCH),x86_64)
    DEB_ARCH := amd64
    RPM_ARCH := x86_64
else ifeq ($(MACHINE_ARCH),aarch64)
    DEB_ARCH := arm64
    RPM_ARCH := aarch64
else ifeq ($(MACHINE_ARCH),armv7l)
    DEB_ARCH := armhf
    RPM_ARCH := armv7hl
else
    # Fallback to machine arch for unknown architectures
    DEB_ARCH := $(MACHINE_ARCH)
    RPM_ARCH := $(MACHINE_ARCH)
endif

# Select architecture based on OS
ifeq ($(OS_ID),debian)
    PACKAGE_ARCH := $(DEB_ARCH)
else ifeq ($(OS_ID),ubuntu)
    PACKAGE_ARCH := $(DEB_ARCH)
else ifeq ($(OS_ID),amzn)
    PACKAGE_ARCH := $(RPM_ARCH)
else ifeq ($(OS_ID),fedora)
    PACKAGE_ARCH := $(RPM_ARCH)
else
    PACKAGE_ARCH := $(MACHINE_ARCH)
endif

# Common package build prerequisites check
define check-package-prereqs
	@if [ ! -f ".vMAJOR" ] || [ ! -f ".vMINOR" ] || [ ! -f ".vPATCH" ]; then \
		echo "Error: Version files (.vMAJOR, .vMINOR, .vPATCH) not found"; \
		exit 1; \
	fi
	@if [ ! -d "$(BIN_DIR)" ]; then \
		echo "Error: Binary directory $(BIN_DIR) not found. Run 'make release' first."; \
		exit 1; \
	fi
	@mkdir -p $(PACKAGE_OUT_DIR)
endef

# Tool availability checking
define check-tool
	@command -v $(1) >/dev/null 2>&1 || { \
		echo "Error: Required tool '$(1)' is not installed."; \
		echo "$(2)"; \
		exit 1; \
	}
endef

# Check for all required build tools
define check-build-tools
	$(call check-tool,$(CC),Please install C compiler ($(CC)))
	$(call check-tool,make,Please install GNU Make)
	$(call check-tool,git,Please install Git version control)
	@echo "Detected compiler type: $(COMPILER_TYPE) ($(CC))"
endef

# Check for package building tools
define check-package-tools
	@if [ "$(PACKAGE_TYPE)" = "deb" ]; then \
		command -v dpkg-deb >/dev/null 2>&1 || { \
			echo "Error: dpkg-deb is required for building Debian packages."; \
			echo "Install with: sudo apt-get install dpkg-dev"; \
			exit 1; \
		}; \
	elif [ "$(PACKAGE_TYPE)" = "rpm" ]; then \
		command -v rpmbuild >/dev/null 2>&1 || { \
			echo "Error: rpmbuild is required for building RPM packages."; \
			echo "Install with: sudo yum install rpm-build"; \
			exit 1; \
		}; \
	fi
endef

# ============================================================================
# File Definitions
# ============================================================================

# Source files (excluding main files)
COMMON_SRCS = $(SRC_DIR)/message.c $(SRC_DIR)/dtls.c $(SRC_DIR)/generic_queue.c $(SRC_DIR)/message_queue.c
COMMON_OBJS_DEBUG = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/$(OS_DIR)/debug/%.o,$(COMMON_SRCS))
COMMON_OBJS_RELEASE = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/$(OS_DIR)/release/%.o,$(COMMON_SRCS))
COMMON_OBJS_TSAN = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/$(OS_DIR)/tsan/%.o,$(COMMON_SRCS))

# Main source files
SERVER_MAIN = $(SRC_DIR)/server.c
CLIENT_MAIN = $(SRC_DIR)/client.c

# Object files for debug and release builds
SERVER_OBJ_DEBUG = $(OBJ_DIR)/$(OS_DIR)/debug/server.o
CLIENT_OBJ_DEBUG = $(OBJ_DIR)/$(OS_DIR)/debug/client.o
SERVER_OBJ_RELEASE = $(OBJ_DIR)/$(OS_DIR)/release/server.o
CLIENT_OBJ_RELEASE = $(OBJ_DIR)/$(OS_DIR)/release/client.o
SERVER_OBJ_TSAN = $(OBJ_DIR)/$(OS_DIR)/tsan/server.o
CLIENT_OBJ_TSAN = $(OBJ_DIR)/$(OS_DIR)/tsan/client.o

# All objects needed for executables
SERVER_OBJS_DEBUG = $(SERVER_OBJ_DEBUG) $(OBJ_DIR)/$(OS_DIR)/debug/message.o $(OBJ_DIR)/$(OS_DIR)/debug/dtls.o
CLIENT_OBJS_DEBUG = $(CLIENT_OBJ_DEBUG)
SERVER_OBJS_RELEASE = $(SERVER_OBJ_RELEASE) $(OBJ_DIR)/$(OS_DIR)/release/message.o $(OBJ_DIR)/$(OS_DIR)/release/dtls.o
CLIENT_OBJS_RELEASE = $(CLIENT_OBJ_RELEASE)
SERVER_OBJS_TSAN = $(SERVER_OBJ_TSAN) $(OBJ_DIR)/$(OS_DIR)/tsan/message.o $(OBJ_DIR)/$(OS_DIR)/tsan/dtls.o
CLIENT_OBJS_TSAN = $(CLIENT_OBJ_TSAN)

# Test files
TEST_SRCS = $(wildcard $(TST_DIR)/test_*.c)
TEST_BINS = $(patsubst $(TST_DIR)/%.c,$(BIN_DIR_OS)/sc-%,$(TEST_SRCS))
TEST_OBJS = $(patsubst $(TST_DIR)/%.c,$(OBJ_DIR)/$(OS_DIR)/%.o,$(TEST_SRCS))

# Unity test framework
UNITY_OBJ = $(OBJ_DIR)/$(OS_DIR)/unity.o

# Dependency files (generated by -MMD -MP)
DEPS = $(COMMON_OBJS_DEBUG:.o=.d) $(COMMON_OBJS_RELEASE:.o=.d) $(COMMON_OBJS_TSAN:.o=.d) \
       $(SERVER_OBJ_DEBUG:.o=.d) $(CLIENT_OBJ_DEBUG:.o=.d) \
       $(SERVER_OBJ_RELEASE:.o=.d) $(CLIENT_OBJ_RELEASE:.o=.d) \
       $(SERVER_OBJ_TSAN:.o=.d) $(CLIENT_OBJ_TSAN:.o=.d) \
       $(TEST_OBJS:.o=.d) $(UNITY_OBJ:.o=.d)

# ============================================================================
# Primary Targets
# ============================================================================

# Default target - build debug versions
.PHONY: all
all: check-tools mbedtls $(BIN_DIR_OS)/sc-server $(BIN_DIR_OS)/sc-client

# Check all required tools
.PHONY: check-tools
check-tools:
	@echo "Checking for required tools..."
	$(check-build-tools)
	@echo "All required build tools are available"

# Check all optional tools
.PHONY: check-all-tools
check-all-tools: check-tools
	@echo ""
	@echo "Checking optional tools..."
	@echo -n "cmake: "; command -v cmake >/dev/null 2>&1 && echo "found" || echo "NOT FOUND (required for mbedTLS)"
	@echo -n "docker: "; command -v docker >/dev/null 2>&1 && echo "found" || echo "NOT FOUND (required for container builds)"
	@echo -n "aws: "; command -v aws >/dev/null 2>&1 && echo "found" || echo "NOT FOUND (required for pull-amazon)"
	@echo -n "dpkg-deb: "; command -v dpkg-deb >/dev/null 2>&1 && echo "found" || echo "NOT FOUND (required for Debian packages)"
	@echo -n "rpmbuild: "; command -v rpmbuild >/dev/null 2>&1 && echo "found" || echo "NOT FOUND (required for RPM packages)"
	@echo -n "clang-format: "; command -v clang-format >/dev/null 2>&1 && echo "found" || echo "NOT FOUND (required for code formatting)"
	@echo -n "openssl: "; command -v openssl >/dev/null 2>&1 && echo "found" || echo "NOT FOUND (required for certificates)"
	@echo -n "npm: "; command -v npm >/dev/null 2>&1 && echo "found" || echo "NOT FOUND (required for CLI tools)"
	@echo -n "dot: "; command -v dot >/dev/null 2>&1 && echo "found" || echo "NOT FOUND (required for architecture diagrams)"
	@echo -n "gdb: "; command -v gdb >/dev/null 2>&1 && echo "found" || echo "NOT FOUND (optional for debugging)"
	@echo ""

# Build individual debug targets
.PHONY: server
server: mbedtls $(BIN_DIR_OS)/sc-server

.PHONY: client
client: mbedtls $(BIN_DIR_OS)/sc-client

# ============================================================================
# Vendor Dependencies
# ============================================================================

# Ensure mbedTLS source is available
.PHONY: clone-mbedtls
clone-mbedtls:
	@if [ ! -d "$(DEPS_SRC_DIR)/mbedtls" ]; then \
		echo "Cloning mbedTLS $(MBEDTLS_VERSION)..."; \
		mkdir -p $(DEPS_SRC_DIR); \
		git clone --depth 1 --branch $(MBEDTLS_VERSION) https://github.com/Mbed-TLS/mbedtls.git $(DEPS_SRC_DIR)/mbedtls; \
		cd $(DEPS_SRC_DIR)/mbedtls && git config advice.detachedHead false; \
	else \
		echo "mbedTLS source already exists in $(DEPS_SRC_DIR)/mbedtls"; \
	fi

# External dependency compiler flags
# Use minimal, functional flags for external dependencies since we don't control their code
# These flags ensure compatibility and optimization without strict warnings
# -Wno-error explicitly disables treating warnings as errors in external code
EXTERNAL_DEPS_CFLAGS = -O2 -fPIC -D_DEFAULT_SOURCE -Wno-error

# Build mbedTLS in deps/build/<os>
.PHONY: mbedtls
mbedtls: clone-mbedtls
	$(call check-tool,cmake,Please install CMake - required for building mbedTLS)
	@if [ ! -f "$(DEPS_BUILD_DIR)/$(OS_DIR)/lib/libmbedtls.so" ]; then \
		if [ ! -d "$(DEPS_SRC_DIR)/mbedtls" ]; then \
			echo "Error: $(DEPS_SRC_DIR)/mbedtls not found. Run 'make clone-mbedtls' on the host first."; \
			exit 1; \
		fi; \
		echo "Building mbedTLS for $(OS_DIR)..."; \
		echo "Cleaning mbedTLS source directory..."; \
		cd $(DEPS_SRC_DIR)/mbedtls && make clean 2>/dev/null || true && cd $(PWD); \
		mkdir -p $(DEPS_BUILD_DIR)/$(OS_DIR)/build-mbedtls; \
		cd $(DEPS_BUILD_DIR)/$(OS_DIR)/build-mbedtls && \
		CFLAGS="$(EXTERNAL_DEPS_CFLAGS)" \
		cmake -DCMAKE_INSTALL_PREFIX=$(PWD)/$(DEPS_BUILD_DIR)/$(OS_DIR) \
		      -DCMAKE_C_FLAGS="$(EXTERNAL_DEPS_CFLAGS)" \
		      -DUSE_SHARED_MBEDTLS_LIBRARY=On \
		      -DMBEDTLS_FATAL_WARNINGS=OFF \
		      -DENABLE_TESTING=OFF \
		      $(PWD)/$(DEPS_SRC_DIR)/mbedtls && \
		make -j$$(nproc) && \
		make install && \
		cd $(PWD) && \
		rm -rf $(DEPS_BUILD_DIR)/$(OS_DIR)/build-mbedtls; \
	else \
		echo "mbedTLS already built in $(DEPS_BUILD_DIR)/$(OS_DIR)"; \
	fi

# Ensure Unity source is available
.PHONY: clone-unity
clone-unity:
	@if [ ! -d "$(DEPS_SRC_DIR)/unity" ]; then \
		echo "Cloning Unity $(UNITY_VERSION)..."; \
		mkdir -p $(DEPS_SRC_DIR); \
		git clone --depth 1 --branch $(UNITY_VERSION) https://github.com/ThrowTheSwitch/Unity.git $(DEPS_SRC_DIR)/unity; \
		cd $(DEPS_SRC_DIR)/unity && git config advice.detachedHead false; \
	else \
		echo "Unity source already exists in $(DEPS_SRC_DIR)/unity"; \
	fi

# Build release versions
.PHONY: release
release:
	@echo "Building release for $(OS_ID) ($(OS_DIR))..."
	@$(MAKE) mbedtls
	@$(MAKE) CFLAGS="$(CFLAGS_RELEASE)" $(BIN_DIR_OS)/sc-server-release $(BIN_DIR_OS)/sc-client-release
	@echo "Release build complete for $(OS_ID)"
	@echo "Binaries: $(BIN_DIR_OS)/sc-server-release -> $$(readlink $(BIN_DIR_OS)/sc-server-release)"
	@echo "          $(BIN_DIR_OS)/sc-client-release -> $$(readlink $(BIN_DIR_OS)/sc-client-release)"

# ============================================================================
# Build Rules - Debug
# ============================================================================

# Debug executables
$(BIN_DIR_OS)/sc-server: $(SERVER_OBJS_DEBUG) | $(BIN_DIR_OS)
	$(CC) -o $@ $(SERVER_OBJS_DEBUG) $(LDFLAGS)
	@if [ "$(IS_HOST_BUILD)" = "1" ]; then \
		ln -f $@ $(BIN_DIR)/ 2>/dev/null || true; \
	fi

$(BIN_DIR_OS)/sc-client: $(CLIENT_OBJS_DEBUG) | $(BIN_DIR_OS)
	$(CC) -o $@ $(CLIENT_OBJS_DEBUG) $(LDFLAGS)
	@if [ "$(IS_HOST_BUILD)" = "1" ]; then \
		ln -f $@ $(BIN_DIR)/ 2>/dev/null || true; \
	fi

# Debug object files - generic rule
# Note: All debug builds use -march=native to optimize for the build machine
$(OBJ_DIR)/$(OS_DIR)/debug/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)/$(OS_DIR)/debug
	$(CC) $(CFLAGS_DEBUG) -march=native -I$(SRC_DIR) -c -o $@ $<

# ============================================================================
# Build Rules - Release
# ============================================================================

# Release binary versioning helper
define build-release-binary
	@VERSION=$$($(get-version)); \
	$(CC) -o $(BIN_DIR_OS)/$(1)-$$VERSION $(2) $(LDFLAGS_RELEASE); \
	ln -sf $(1)-$$VERSION $(BIN_DIR_OS)/$(1)-release; \
	if [ "$(IS_HOST_BUILD)" = "1" ]; then \
		ln -f $(BIN_DIR_OS)/$(1)-$$VERSION $(BIN_DIR)/ 2>/dev/null || true; \
		ln -sf $(1)-$$VERSION $(BIN_DIR)/$(1)-release 2>/dev/null || true; \
	fi
endef

# Release executables (with versioning)
$(BIN_DIR_OS)/sc-server-release: $(SERVER_OBJS_RELEASE) | $(BIN_DIR_OS)
	$(call build-release-binary,sc-server,$^)

$(BIN_DIR_OS)/sc-client-release: $(CLIENT_OBJS_RELEASE) | $(BIN_DIR_OS)
	$(call build-release-binary,sc-client,$^)

# Release object files - generic rule
$(OBJ_DIR)/$(OS_DIR)/release/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)/$(OS_DIR)/release
	$(CC) $(CFLAGS_RELEASE) -I$(SRC_DIR) -c -o $@ $<

# Release server with x86-64-v4 architecture (requires AVX-512)
$(OBJ_DIR)/$(OS_DIR)/release/server.o: $(SRC_DIR)/server.c | $(OBJ_DIR)/$(OS_DIR)/release
	$(CC) $(CFLAGS_RELEASE) -march=x86-64-v4 -I$(SRC_DIR) -c -o $@ $<

# Release client with x86-64-v3 architecture (requires AVX2)
$(OBJ_DIR)/$(OS_DIR)/release/client.o: $(SRC_DIR)/client.c | $(OBJ_DIR)/$(OS_DIR)/release
	$(CC) $(CFLAGS_RELEASE) -march=x86-64-v3 -I$(SRC_DIR) -c -o $@ $<

# ============================================================================
# Test Targets
# ============================================================================

.PHONY: tests
tests: mbedtls $(TEST_BINS)

.PHONY: run-tests
run-tests: mbedtls tests server client
	@for test in $(TEST_BINS); do \
		echo "Running $$test..."; \
		ASAN_OPTIONS=allocator_may_return_null=1 $$test || exit 1; \
	done

# Unity framework object
$(UNITY_OBJ): $(DEPS_SRC_DIR)/unity/src/unity.c | $(OBJ_DIR)/$(OS_DIR) clone-unity
	$(CC) $(EXTERNAL_DEPS_CFLAGS) -I$(DEPS_SRC_DIR)/unity/src -c -o $@ $<

# Test executable link command
define link-test
	$(CC) -o $@ $^ $(LDFLAGS)
	@if [ "$(IS_HOST_BUILD)" = "1" ]; then \
		ln -f $@ $(BIN_DIR)/ 2>/dev/null || true; \
	fi
endef

# Function to get module name from test name
# Default: remove test_ prefix (e.g., test_message -> message)
# Special case: test_server depends on dtls module
get-test-module = $(if $(filter test_server,$(1)),dtls,$(patsubst test_%,%,$(1)))

# Generic test rule generator
define test-rule
$(BIN_DIR_OS)/sc-$(1): $(OBJ_DIR)/$(OS_DIR)/$(1).o $(UNITY_OBJ) $(OBJ_DIR)/$(OS_DIR)/debug/$(call get-test-module,$(1)).o | $(BIN_DIR_OS)
	$$(link-test)
endef

# Generate rules for all tests
$(foreach test,$(basename $(notdir $(TEST_SRCS))),$(eval $(call test-rule,$(test))))

# Test object files
$(OBJ_DIR)/$(OS_DIR)/test_%.o: $(TST_DIR)/test_%.c | $(OBJ_DIR)/$(OS_DIR) clone-unity
	$(CC) $(CFLAGS_DEBUG) -I$(SRC_DIR) -I$(DEPS_SRC_DIR)/unity/src -c -o $@ $<

# ============================================================================
# ThreadSanitizer Targets
# ============================================================================

# TSAN object file compilation rules
# Note: All TSAN builds use -march=native to optimize for the build machine
$(OBJ_DIR)/$(OS_DIR)/tsan/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)/$(OS_DIR)/tsan
	$(CC) $(CFLAGS_TSAN) -march=native -I$(SRC_DIR) -c -o $@ $<

# TSAN test object files
$(OBJ_DIR)/$(OS_DIR)/tsan/test_%.o: $(TST_DIR)/test_%.c | $(OBJ_DIR)/$(OS_DIR)/tsan clone-unity
	$(CC) $(CFLAGS_TSAN) -march=native -I$(SRC_DIR) -I$(DEPS_SRC_DIR)/unity/src -c -o $@ $<

# TSAN Unity object
$(OBJ_DIR)/$(OS_DIR)/tsan/unity.o: $(DEPS_SRC_DIR)/unity/src/unity.c | $(OBJ_DIR)/$(OS_DIR)/tsan clone-unity
	$(CC) $(EXTERNAL_DEPS_CFLAGS) -fsanitize=thread -I$(DEPS_SRC_DIR)/unity/src -c -o $@ $<

# TSAN executables
$(BIN_DIR_OS)/sc-server-tsan: $(SERVER_OBJS_TSAN) | $(BIN_DIR_OS)
	$(CC) -o $@ $(SERVER_OBJS_TSAN) $(LDFLAGS_TSAN)
	@if [ "$(IS_HOST_BUILD)" = "1" ]; then \
		ln -f $@ $(BIN_DIR)/ 2>/dev/null || true; \
	fi

$(BIN_DIR_OS)/sc-client-tsan: $(CLIENT_OBJS_TSAN) | $(BIN_DIR_OS)
	$(CC) -o $@ $(CLIENT_OBJS_TSAN) $(LDFLAGS_TSAN)
	@if [ "$(IS_HOST_BUILD)" = "1" ]; then \
		ln -f $@ $(BIN_DIR)/ 2>/dev/null || true; \
	fi

# TSAN test executables
TEST_BINS_TSAN = $(patsubst $(TST_DIR)/%.c,$(BIN_DIR_OS)/sc-%-tsan,$(TEST_SRCS))

# Helper function to build tsan test with proper module dependencies
define link-test-tsan
	$(CC) -o $@ $^ $(LDFLAGS_TSAN)
	@if [ "$(IS_HOST_BUILD)" = "1" ]; then \
		ln -f $@ $(BIN_DIR)/ 2>/dev/null || true; \
	fi
endef

# Generic queue tests (no dependencies)
$(BIN_DIR_OS)/sc-test_generic_queue-tsan: $(OBJ_DIR)/$(OS_DIR)/tsan/test_generic_queue.o $(OBJ_DIR)/$(OS_DIR)/tsan/unity.o $(OBJ_DIR)/$(OS_DIR)/tsan/generic_queue.o
	$(call link-test-tsan)

# Message tests  
$(BIN_DIR_OS)/sc-test_message-tsan: $(OBJ_DIR)/$(OS_DIR)/tsan/test_message.o $(OBJ_DIR)/$(OS_DIR)/tsan/unity.o $(OBJ_DIR)/$(OS_DIR)/tsan/message.o
	$(call link-test-tsan)

# DTLS tests
$(BIN_DIR_OS)/sc-test_dtls-tsan: $(OBJ_DIR)/$(OS_DIR)/tsan/test_dtls.o $(OBJ_DIR)/$(OS_DIR)/tsan/unity.o $(OBJ_DIR)/$(OS_DIR)/tsan/dtls.o
	$(call link-test-tsan)

# Server tests (uses DTLS but not full server)
$(BIN_DIR_OS)/sc-test_server-tsan: $(OBJ_DIR)/$(OS_DIR)/tsan/test_server.o $(OBJ_DIR)/$(OS_DIR)/tsan/unity.o $(OBJ_DIR)/$(OS_DIR)/tsan/dtls.o
	$(call link-test-tsan)

.PHONY: tsan
tsan: mbedtls $(BIN_DIR_OS)/sc-server-tsan $(BIN_DIR_OS)/sc-client-tsan $(TEST_BINS_TSAN)

.PHONY: check-tsan
check-tsan: tsan
	@echo "Running tests with ThreadSanitizer..."
	@for test in $(TEST_BINS_TSAN); do \
		echo "Running $$test..."; \
		TSAN_OPTIONS=allocator_may_return_null=1 $$test || exit 1; \
	done
	@echo "All ThreadSanitizer tests passed!"

# ============================================================================
# Development Targets
# ============================================================================

# Run targets
.PHONY: run-server
run-server: server | $(DAT_DIR)
	$(BIN_DIR_OS)/sc-server

.PHONY: run-client
run-client: client
	$(BIN_DIR_OS)/sc-client

# Debug with GDB
.PHONY: debug-server
debug-server: server
	gdb $(BIN_DIR_OS)/sc-server

.PHONY: debug-client
debug-client: client
	gdb $(BIN_DIR_OS)/sc-client

# Code formatting
.PHONY: fmt
fmt:
	$(call check-tool,clang-format,Please install clang-format for code formatting)
	@find . \( -name "*.c" -o -name "*.h" \) -type f \
		-not -path "./deps/*" \
		-not -path "./tests/vendor/*" \
		-not -path "./obj/*" \
		-not -path "./bin/*" \
		-not -path "./pkg/*" \
		| while read file; do \
		echo "Formatting: $$file"; \
		clang-format -i "$$file"; \
	done

# Generate self-signed certificates for DTLS
.PHONY: certs
certs:
	$(call check-tool,openssl,Please install OpenSSL for certificate generation)
	@if [ -f ".secrets/certs/server.key" ] && [ -f ".secrets/certs/server.crt" ]; then \
		echo "Certificates already exist in .secrets/certs/ (delete them to regenerate)"; \
	else \
		echo "Generating self-signed certificates..."; \
		mkdir -p .secrets/certs; \
		openssl req -x509 -newkey rsa:4096 -keyout .secrets/certs/server.key -out .secrets/certs/server.crt -sha256 -days 365 -nodes -subj "/CN=localhost"; \
		echo "Certificates generated in .secrets/certs/"; \
		echo "  - .secrets/certs/server.key (private key)"; \
		echo "  - .secrets/certs/server.crt (certificate)"; \
	fi

# Show certificate information
.PHONY: certs-info
certs-info:
	@if [ -f ".secrets/certs/server.crt" ]; then \
		echo "Certificate information:"; \
		openssl x509 -in .secrets/certs/server.crt -noout -subject -dates -fingerprint | sed 's/^/  /'; \
	else \
		echo "No certificates found. Run 'make certs' to generate them."; \
	fi

# Show architecture detection details
.PHONY: arch-info
arch-info:
	@echo "Architecture Detection:"
	@echo "  uname -m:     $(MACHINE_ARCH)"
	@echo "  OS detected:  $(OS_ID)"
	@echo ""
	@echo "Package Architecture Mappings:"
	@echo "  x86_64  -> Debian: amd64,    RPM: x86_64"
	@echo "  aarch64 -> Debian: arm64,    RPM: aarch64"
	@echo "  armv7l  -> Debian: armhf,    RPM: armv7hl"
	@echo ""
	@echo "Selected for current OS ($(OS_ID)): $(PACKAGE_ARCH)"

# Show Docker configuration
.PHONY: docker-info
docker-info:
	@echo "Docker Configuration:"
	@echo "  Common options: $(DOCKER_RUN_OPTS)"
	@echo ""
	@echo "Base Images:"
	@echo "  Debian: $(debian_IMAGE) from $(debian_IMAGE_SOURCE)"
	@echo "  Ubuntu: $(ubuntu_IMAGE) from $(ubuntu_IMAGE_SOURCE)"
	@echo "  Amazon: $(amazon_IMAGE) from $(amazon_IMAGE_SOURCE)"
	@echo "  Fedora: $(fedora_IMAGE) from $(fedora_IMAGE_SOURCE)"

# Update CLI tools
.PHONY: update-tools
update-tools:
	$(call check-tool,npm,Please install Node.js and npm - https://nodejs.org)
	npm upgrade -g @google/gemini-cli
	npm upgrade -g @anthropic-ai/claude-code
	npm upgrade -g @openai/codex

# Install CLI tools
.PHONY: install-tools
install-tools:
	$(call check-tool,npm,Please install Node.js and npm - https://nodejs.org)
	npm install -g @google/gemini-cli
	npm install -g @anthropic-ai/claude-code
	npm install -g @openai/codex

# Architecture diagram
# Regenerate when any source files change
SRC_FILES = $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/*.h)

.PHONY: dot
dot: docs/arch.png

docs/arch.png: arch.dot $(SRC_FILES)
	$(call check-tool,dot,Please install Graphviz - sudo apt-get install graphviz)
	dot -Tpng $< -o $@

# ============================================================================
# Installation & Cleanup
# ============================================================================

.PHONY: install
install: release certs
	@scripts/install.sh "$(PREFIX)" "$(BIN_DIR)" "$(OS_DIR)"

.PHONY: uninstall
uninstall:
	@scripts/uninstall.sh "$(PREFIX)"

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)
	# Clean bin directory - remove OS-specific dirs and any files in bin/
	rm -rf $(BIN_DIR)/*/
	rm -f $(BIN_DIR)/*

.PHONY: clean-all
clean-all: clean
	rm -rf $(DEPS_DIR) pkg/*/out

.PHONY: clean-packages
clean-packages:
	rm -rf pkg/*/out

.PHONY: clean-certs
clean-certs:
	@echo "Removing certificates..."
	@rm -f .secrets/certs/server.key .secrets/certs/server.crt
	@rmdir certs 2>/dev/null || true

# ============================================================================
# Docker Configuration
# ============================================================================
# Docker run options
DOCKER_RUN_OPTS = --rm -v $(PWD):/workspace -e USER_ID=$(shell id -u) -e GROUP_ID=$(shell id -g)

# ECR settings for Amazon Linux
ECR_REGISTRY = public.ecr.aws
ECR_REPO = amazonlinux

# Base image definitions
debian_IMAGE = debian:stable-slim
debian_IMAGE_SOURCE = Docker Hub
amazon_IMAGE = amazonlinux:2023
amazon_IMAGE_SOURCE = Amazon ECR
ubuntu_IMAGE = ubuntu:24.04
ubuntu_IMAGE_SOURCE = Docker Hub
fedora_IMAGE = fedora:40
fedora_IMAGE_SOURCE = Docker Hub


# Pull base images for builders
.PHONY: pull-amazon
pull-amazon:
	$(call check-tool,docker,Please install Docker - https://docs.docker.com/get-docker/)
	$(call check-tool,aws,Please install AWS CLI - https://aws.amazon.com/cli/)
	@echo "Logging in to Amazon ECR Public..."
	@aws ecr-public get-login-password --region us-east-1 | docker login --username AWS --password-stdin $(ECR_REGISTRY)
	@echo "Pulling Amazon Linux 2023 image..."
	@docker pull $(ECR_REGISTRY)/$(ECR_REPO)/amazonlinux:2023
	@echo "Tagging as amazonlinux:2023..."
	@docker tag $(ECR_REGISTRY)/$(ECR_REPO)/amazonlinux:2023 amazonlinux:2023
	@echo "Amazon Linux 2023 image pulled successfully"

.PHONY: pull-debian
pull-debian:
	$(call check-tool,docker,Please install Docker - https://docs.docker.com/get-docker/)
	@echo "Pulling Debian slim image from Docker Hub..."
	@docker pull debian:stable-slim
	@echo "Debian slim image pulled successfully"

.PHONY: pull-ubuntu
pull-ubuntu:
	$(call check-tool,docker,Please install Docker - https://docs.docker.com/get-docker/)
	@echo "Pulling Ubuntu image from Docker Hub..."
	@docker pull ubuntu:24.04
	@echo "Ubuntu image pulled successfully"

.PHONY: pull-fedora
pull-fedora:
	$(call check-tool,docker,Please install Docker - https://docs.docker.com/get-docker/)
	@echo "Pulling Fedora image from Docker Hub..."
	@docker pull fedora:40
	@echo "Fedora image pulled successfully"

# Pattern rule for building Docker images
.PHONY: build-%
build-%:
	$(call check-tool,docker,Please install Docker - https://docs.docker.com/get-docker/)
	@echo "Building Space Captain $* builder image..."
	@docker build -f Dockerfile.$* -t space-captain-$*-builder .
	@echo "Builder image created successfully"

# Pattern rule for interactive shell in Docker builder
.PHONY: shell-%
shell-%: build-%
	$(call check-tool,docker,Please install Docker - https://docs.docker.com/get-docker/)
	@echo "Starting interactive shell in $* builder container..."
	@docker run -it --volume "$$(pwd)":/workspace --workdir=/workspace space-captain-$*-builder /bin/bash

# Smart package targets that auto-detect Docker environment
.PHONY: package-debian
package-debian:
	@if [ -f /.dockerenv ] || [ -n "$$(grep -q docker /proc/1/cgroup 2>/dev/null && echo 1)" ]; then \
		echo "Running in Docker container - building package..."; \
		$(MAKE) package-deb; \
	else \
		echo "Not in Docker - launching build in Debian container..."; \
		$(MAKE) build-debian; \
		docker run --rm --volume "$$(pwd)":/workspace --workdir=/workspace \
			-e USER_ID=$$(id -u) -e GROUP_ID=$$(id -g) \
			space-captain-debian-builder make package-debian; \
	fi

.PHONY: package-amazon
package-amazon:
	@if [ -f /.dockerenv ] || [ -n "$$(grep -q docker /proc/1/cgroup 2>/dev/null && echo 1)" ]; then \
		echo "Running in Docker container - building package..."; \
		$(MAKE) package-rpm; \
	else \
		echo "Not in Docker - launching build in Amazon container..."; \
		$(MAKE) build-amazon; \
		docker run --rm --volume "$$(pwd)":/workspace --workdir=/workspace \
			-e USER_ID=$$(id -u) -e GROUP_ID=$$(id -g) \
			space-captain-amazon-builder make package-amazon; \
	fi

.PHONY: package-ubuntu
package-ubuntu:
	@if [ -f /.dockerenv ] || [ -n "$$(grep -q docker /proc/1/cgroup 2>/dev/null && echo 1)" ]; then \
		echo "Running in Docker container - building package..."; \
		$(MAKE) package-deb; \
	else \
		echo "Not in Docker - launching build in Ubuntu container..."; \
		$(MAKE) build-ubuntu; \
		docker run --rm --volume "$$(pwd)":/workspace --workdir=/workspace \
			-e USER_ID=$$(id -u) -e GROUP_ID=$$(id -g) \
			space-captain-ubuntu-builder make package-ubuntu; \
	fi

.PHONY: package-fedora
package-fedora:
	@if [ -f /.dockerenv ] || [ -n "$$(grep -q docker /proc/1/cgroup 2>/dev/null && echo 1)" ]; then \
		echo "Running in Docker container - building package..."; \
		$(MAKE) package-rpm; \
	else \
		echo "Not in Docker - launching build in Fedora container..."; \
		$(MAKE) build-fedora; \
		docker run --rm --volume "$$(pwd)":/workspace --workdir=/workspace \
			-e USER_ID=$$(id -u) -e GROUP_ID=$$(id -g) \
			space-captain-fedora-builder make package-fedora; \
	fi

# ============================================================================
# Version Management
# ============================================================================

.PHONY: version
version:
	@$(get-version)

.PHONY: deps
deps:
	@echo "System Dependencies"
	@echo "==================="
	@echo ""
	@echo "Required packages:"
	@echo "  - gcc"
	@echo "  - make"
	@echo "  - git"
	@echo "  - cmake"
	@echo "  - openssl"
	@echo "  - AddressSanitizer library"
	@echo "  - UndefinedBehaviorSanitizer library"
	@echo ""
	@echo "Optional packages:"
	@echo "  - python3 (for mbedTLS configuration checks and tests)"
	@echo "  - clang-format"
	@echo "  - gdb"
	@echo "  - graphviz"
	@echo "  - docker"
	@echo "  - dpkg-dev (for Debian packaging)"
	@echo "  - rpm-build (for RPM packaging)"
	@echo "  - chrpath (for RPATH stripping)"
	@echo ""
	@echo "External libraries (fetched automatically):"
	@echo "  - mbedTLS $(MBEDTLS_VERSION)"
	@echo "  - Unity Test Framework $(UNITY_VERSION)"

.PHONY: package-info
package-info:
	@echo "Package Information:"
	@echo "  Name:        $(PACKAGE_NAME)"
	@echo "  Version:     $(PACKAGE_VERSION)"
	@echo "  Description: $(PACKAGE_DESCRIPTION)"
	@echo "  Vendor:      $(PACKAGE_VENDOR)"
	@echo "  License:     $(PACKAGE_LICENSE)"
	@echo "  Maintainer:  $(PACKAGE_MAINTAINER)"
	@echo "  URL:         $(PACKAGE_URL)"
	@echo ""
	@echo "Architecture:"
	@echo "  Machine:     $(MACHINE_ARCH)"
	@echo "  Debian:      $(DEB_ARCH)"
	@echo "  RPM:         $(RPM_ARCH)"
	@echo "  Current OS:  $(OS_ID) -> $(PACKAGE_ARCH)"
	@echo ""
	@echo "  Output Dir:  $(PACKAGE_OUT_DIR)/"

# Version bump helper function
define bump-version
	@MAJOR=$$(cat .vMAJOR); \
	MINOR=$$(cat .vMINOR); \
	PATCH=$$(cat .vPATCH); \
	$(1); \
	echo "Version bumped to $$($(get-version))"
endef

.PHONY: major-bump
major-bump:
	$(call bump-version,NEW_MAJOR=$$(($$MAJOR + 1)); echo $$NEW_MAJOR > .vMAJOR; echo 0 > .vMINOR; echo 0 > .vPATCH; echo "" > .vPRE)

.PHONY: minor-bump
minor-bump:
	$(call bump-version,NEW_MINOR=$$(($$MINOR + 1)); echo $$NEW_MINOR > .vMINOR; echo 0 > .vPATCH; echo "" > .vPRE)

.PHONY: patch-bump
patch-bump:
	$(call bump-version,NEW_PATCH=$$(($$PATCH + 1)); echo $$NEW_PATCH > .vPATCH)

.PHONY: pre-bump
pre-bump:
	@PRE=$$(cat .vPRE 2>/dev/null || true); \
	if [ -z "$$PRE" ]; then PRE=0; fi; \
	NEW_PRE=$$(($$PRE + 1)); \
	echo "$$NEW_PRE" > .vPRE; \
	echo "Pre-release version bumped to $$NEW_PRE"; \
	echo "New version: $$($(get-version))"

.PHONY: pre-reset
pre-reset:
	@echo "" > .vPRE; \
	echo "Pre-release version reset"; \
	echo "New version: $$($(get-version))"

.PHONY: rel-bump
rel-bump:
	@RELEASE=$$(cat .vREL 2>/dev/null || true); \
	if [ -z "$$RELEASE" ]; then RELEASE=0; fi; \
	NEW_RELEASE=$$(($$RELEASE + 1)); \
	echo "$$NEW_RELEASE" > .vREL; \
	echo "Release bumped to $$NEW_RELEASE"

.PHONY: rel-reset
rel-reset:
	@echo "1" > .vREL; \
	echo "Release reset to 1"


# ============================================================================
# Packaging
# ============================================================================

# Auto-detect OS and build appropriate package
.PHONY: package
package:
	@echo "Auto-detecting OS for package build..."; \
	if [ "$(OS_ID)" = "debian" ]; then \
		echo "Detected Debian - building DEB package"; \
		$(MAKE) package-debian; \
	elif [ "$(OS_ID)" = "ubuntu" ]; then \
		echo "Detected Ubuntu - building DEB package"; \
		$(MAKE) package-ubuntu; \
	elif [ "$(OS_ID)" = "amzn" ]; then \
		echo "Detected Amazon Linux - building RPM package"; \
		$(MAKE) package-amazon; \
	elif [ "$(OS_ID)" = "fedora" ]; then \
		echo "Detected Fedora - building RPM package"; \
		$(MAKE) package-fedora; \
	else \
		echo "Error: Unsupported OS '$(OS_ID)' for package building"; \
		echo "Supported: debian, ubuntu, amzn, fedora"; \
		exit 1; \
	fi

# Build packages for all supported operating systems (host only)
.PHONY: packages
packages:
	@if [ -f /.dockerenv ] || [ -n "$$(grep -q docker /proc/1/cgroup 2>/dev/null && echo 1)" ]; then \
		echo "Error: 'make packages' should only be run from the host, not inside Docker"; \
		echo "Use 'make package-<os>' to build a specific package inside Docker"; \
		exit 1; \
	else \
		echo "Building packages for all supported operating systems..."; \
		echo ""; \
		echo "=== Building Debian package ==="; \
		$(MAKE) package-debian; \
		echo ""; \
		echo "=== Building Ubuntu package ==="; \
		$(MAKE) package-ubuntu; \
		echo ""; \
		echo "=== Building Amazon Linux package ==="; \
		$(MAKE) package-amazon; \
		echo ""; \
		echo "=== Building Fedora package ==="; \
		$(MAKE) package-fedora; \
		echo ""; \
		echo "All packages built successfully!"; \
	fi

.PHONY: package-deb
package-deb: release certs
	$(check-package-prereqs)
	$(call check-tool,dpkg-deb,Install with: sudo apt-get install dpkg-dev)
	@GIT_SHA=$$(git rev-parse --short HEAD 2>/dev/null || echo "nogit"); \
	FULL_VERSION=$$($(get-version)); \
	SAFE_VERSION=$$(echo "$$FULL_VERSION" | sed 's/-/~/g'); \
	echo "Building Debian package v$$FULL_VERSION for $(DEB_ARCH)..."; \
	scripts/build-deb-package.sh "$$SAFE_VERSION" "$(DEB_ARCH)" "$(BIN_DIR)" "$$GIT_SHA" "$(OS_DIR)"; \
	RELEASE=$$(cat .vREL 2>/dev/null || echo 1); \
	echo "Debian package created: $(PACKAGE_OUT_DIR)/$(PACKAGE_NAME)_$$SAFE_VERSION-$$RELEASE_$(DEB_ARCH).deb"

# RPM package for Amazon Linux / RHEL systems
.PHONY: package-rpm
package-rpm: release certs
	$(check-package-prereqs)
	$(call check-tool,rpmbuild,Install with: sudo yum install rpm-build)
	@GIT_SHA=$$(git rev-parse --short HEAD 2>/dev/null || echo "nogit"); \
	FULL_VERSION=$$($(get-version)); \
	SAFE_VERSION=$$(echo "$$FULL_VERSION" | sed 's/-/~/g'); \
	echo "Building RPM package v$$FULL_VERSION for $(RPM_ARCH)..."; \
	scripts/build-rpm-package.sh "$$SAFE_VERSION" "$(RPM_ARCH)" "$(BIN_DIR)" "$$GIT_SHA" "$(OS_DIR)"; \
	RELEASE=$$(cat .vREL 2>/dev/null || echo 1); \
	echo "RPM package created: $(PACKAGE_OUT_DIR)/$(PACKAGE_NAME)-$$SAFE_VERSION-$$RELEASE.$(RPM_ARCH).rpm"


# ============================================================================
# Infrastructure Targets
# ============================================================================

.PHONY: infra-up
infra-up:
	@echo "Deploying infrastructure..."
	cd infra && terraform apply

.PHONY: infra-down
infra-down:
	@echo "Destroying infrastructure..."
	cd infra && terraform destroy

.PHONY: deploy-server
deploy-server:
	@echo "Deploying latest RPM to server..."
	./infra/bin/deploy-server

.PHONY: deploy-clients
deploy-clients:
	@echo "TODO: Client deployment not yet implemented"
	@echo "      Need to create infra/bin/deploy-clients script"

.PHONY: deploy-telemetry
deploy-telemetry:
	@echo "TODO: Telemetry deployment not yet implemented"
	@echo "      Need to create infra/bin/deploy-telemetry script"

.PHONY: deploy-all
deploy-all: deploy-server deploy-clients deploy-telemetry
	@echo "All deployments complete!"

.PHONY: infra-status
infra-status:
	@echo "Checking infrastructure status..."
	./infra/bin/ssh-info


# ============================================================================
# Help Target
# ============================================================================

.PHONY: help
help:
	@echo "Space Captain Makefile"
	@echo "===================="
	@echo ""
	@echo "Building:"
	@echo "  make                 Build debug server and client (default)"
	@echo "  make server          Build debug server only"
	@echo "  make client          Build debug client only"
	@echo "  make release         Build optimized release versions"
	@echo "  make tests           Build all test executables"
	@echo "  make tsan            Build with ThreadSanitizer"
	@echo ""
	@echo "Testing:"
	@echo "  make run-tests       Build and run all tests"
	@echo "  make check-tsan      Run tests with ThreadSanitizer"
	@echo ""
	@echo "Running:"
	@echo "  make run-server      Build and run debug server"
	@echo "  make run-client      Build and run debug client"
	@echo ""
	@echo "Installation:"
	@echo "  make install         Install release versions to PREFIX"
	@echo "  make uninstall       Remove installed files from PREFIX"
	@echo ""
	@echo "Packaging:"
	@echo "  make package         Auto-detect OS and build package"
	@echo "  make packages        Build packages for all OSes (host only)"
	@echo "  make package-<os>    Build package for OS (debian/ubuntu/amazon/fedora)"
	@echo "  make package-deb     Build Debian package (.deb)"
	@echo "  make package-rpm     Build RPM package (.rpm)"
	@echo ""
	@echo "Cleaning:"
	@echo "  make clean           Remove build artifacts (keeps deps)"
	@echo "  make clean-all       Remove all artifacts including deps"
	@echo "  make clean-certs     Remove generated certificates"
	@echo ""
	@echo "Development:"
	@echo "  make fmt             Format all code with clang-format"
	@echo "  make debug-server    Debug server with GDB"
	@echo "  make debug-client    Debug client with GDB"
	@echo "  make certs           Generate self-signed certificates (idempotent)"
	@echo "  make certs-info      Display certificate information"
	@echo "  make dot             Generate architecture diagram"
	@echo ""
	@echo "Dependencies:"
	@echo "  make deps            Display external dependencies and versions"
	@echo "  make mbedtls         Build mbedTLS in $(DEPS_BUILD_DIR)"
	@echo "  make clone-mbedtls   Clone/update mbedTLS source to $(DEPS_SRC_DIR)"
	@echo "  make clone-unity     Clone/update Unity test framework to $(DEPS_SRC_DIR)"
	@echo ""
	@echo "Docker:"
	@echo "  make shell-<os>      Start interactive shell in builder (debian/ubuntu/amazon/fedora)"
	@echo "  make build-<os>      Build Docker builder image (debian/ubuntu/amazon/fedora)"
	@echo "  make pull-debian     Pull Debian slim Docker image"
	@echo "  make pull-ubuntu     Pull Ubuntu Docker image"
	@echo "  make pull-amazon     Pull Amazon Linux 2023 Docker image"
	@echo "  make pull-fedora     Pull Fedora Docker image"
	@echo "  make docker-info     Display Docker configuration"
	@echo ""
	@echo "Version Management:"
	@echo "  make version         Display current version"
	@echo "  make major-bump      Increment major version (X.0.0)"
	@echo "  make minor-bump      Increment minor version (0.X.0)"
	@echo "  make patch-bump      Increment patch version (0.0.X)"
	@echo "  make pre-bump        Increment pre-release version"
	@echo "  make pre-reset       Reset pre-release version"
	@echo "  make rel-bump        Increment the package release number"
	@echo "  make rel-reset       Reset the package release number"
	@echo "  make package-info    Display package metadata"
	@echo ""
	@echo "Infrastructure:"
	@echo "  make infra-up        Deploy infrastructure with Terraform"
	@echo "  make infra-down      Destroy infrastructure with Terraform"
	@echo "  make infra-status    Show SSH connection info for all instances"
	@echo "  make deploy-all      Deploy to all instances"
	@echo "  make deploy-server   Deploy latest RPM to running server"
	@echo "  make deploy-clients  Deploy latest RPM to running clients (TODO)"
	@echo "  make deploy-telemetry Deploy telemetry stack (TODO)"
	@echo ""
	@echo "Tools:"
	@echo "  make check-tools     Check for required build tools"
	@echo "  make check-all-tools Check for all optional tools"
	@echo "  make install-tools   Install CLI tools (gemini, claude, codex)"
	@echo "  make update-tools    Update CLI tools to latest versions"
	@echo ""
	@echo "Environment Variables:"
	@echo "  PREFIX=<path>        Set installation prefix (default: /usr/local)"

# ============================================================================
# Directory Creation
# ============================================================================

$(BIN_DIR) $(BIN_DIR_OS) $(DAT_DIR) $(OBJ_DIR)/$(OS_DIR)/debug $(OBJ_DIR)/$(OS_DIR)/release $(OBJ_DIR)/$(OS_DIR)/tsan $(OBJ_DIR)/$(OS_DIR):
	mkdir -p $@

# ============================================================================
# Include Dependencies
# ============================================================================

# Include auto-generated dependency files
-include $(DEPS)
