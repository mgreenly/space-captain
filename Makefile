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
	GIT_SHA=$$(git rev-parse --short HEAD 2>/dev/null || echo "nogit"); \
	VERSION="$$MAJOR.$$MINOR.$$PATCH"; \
	if [ -n "$$PRE" ]; then VERSION="$$VERSION-pre$$PRE"; fi; \
	VERSION="$$VERSION+$$GIT_SHA"; \
	echo "$$VERSION"
endef

PACKAGE_VERSION := $(shell $(get-version))
# Package-safe version with '-' replaced by '~' for deb/rpm filenames
PACKAGE_VERSION_SAFE := $(shell $(get-version) | sed 's/-/~/g')
MBEDTLS_VERSION = v2.28.3

# ============================================================================
# Installation Settings
# ============================================================================
HOME := $(shell echo $$HOME)
PREFIX ?= $(HOME)/.local

# ============================================================================
# Compiler Settings
# ============================================================================
CC = gcc
CFLAGS_COMMON = -D_DEFAULT_SOURCE -std=c18 -pedantic -Wall -Wextra -g -MMD -MP -I$(DEPS_BUILD_DIR)/$(OS_DIR)/include
CFLAGS_DEBUG = $(CFLAGS_COMMON) -O0
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O3
CFLAGS = $(CFLAGS_DEBUG)  # Default to debug
LDFLAGS = -L$(DEPS_BUILD_DIR)/$(OS_DIR)/lib -Wl,-rpath,$(PWD)/$(DEPS_BUILD_DIR)/$(OS_DIR)/lib -lpthread -lmbedtls -lmbedx509 -lmbedcrypto

# ============================================================================
# Package Metadata
# ============================================================================
PACKAGE_NAME = space-captain-server
PACKAGE_DESCRIPTION = Space Captain MMO Server
PACKAGE_VENDOR = Space Captain Team
PACKAGE_LICENSE = MIT
PACKAGE_URL = https://github.com/mgreenly/space-captain
PACKAGE_MAINTAINER = mgreenly@gmail.com
PACKAGE_OUT_DIR = pkg/out

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

# OS-specific settings
ifeq ($(OS_ID),debian)
    OS_DIR := debian
    PACKAGE_TYPE := deb
    PACKAGE_BUILDER := dpkg-deb
else ifeq ($(OS_ID),amzn)
    OS_DIR := amazon
    PACKAGE_TYPE := rpm
    PACKAGE_BUILDER := rpmbuild
else
    $(error Unsupported OS: $(OS_ID). This project only supports Debian (ID=debian) and Amazon Linux (ID=amzn))
endif

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
else ifeq ($(OS_ID),amzn)
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
	$(call check-tool,gcc,Please install GCC compiler)
	$(call check-tool,make,Please install GNU Make)
	$(call check-tool,git,Please install Git version control)
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
COMMON_OBJS_DEBUG = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/debug/%.o,$(COMMON_SRCS))
COMMON_OBJS_RELEASE = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/release/%.o,$(COMMON_SRCS))

# Main source files
SERVER_MAIN = $(SRC_DIR)/server.c
CLIENT_MAIN = $(SRC_DIR)/client.c

# Object files for debug and release builds
SERVER_OBJ_DEBUG = $(OBJ_DIR)/debug/server.o
CLIENT_OBJ_DEBUG = $(OBJ_DIR)/debug/client.o
SERVER_OBJ_RELEASE = $(OBJ_DIR)/release/server.o
CLIENT_OBJ_RELEASE = $(OBJ_DIR)/release/client.o

# All objects needed for executables
SERVER_OBJS_DEBUG = $(SERVER_OBJ_DEBUG) $(OBJ_DIR)/debug/message.o $(OBJ_DIR)/debug/dtls.o
CLIENT_OBJS_DEBUG = $(CLIENT_OBJ_DEBUG)
SERVER_OBJS_RELEASE = $(SERVER_OBJ_RELEASE) $(OBJ_DIR)/release/message.o $(OBJ_DIR)/release/dtls.o
CLIENT_OBJS_RELEASE = $(CLIENT_OBJ_RELEASE)

# Test files
TEST_SRCS = $(wildcard $(TST_DIR)/*_tests.c)
TEST_BINS = $(patsubst $(TST_DIR)/%.c,$(BIN_DIR)/sc-%,$(TEST_SRCS))
TEST_OBJS = $(patsubst $(TST_DIR)/%.c,$(OBJ_DIR)/%.o,$(TEST_SRCS))

# Unity test framework
UNITY_OBJ = $(OBJ_DIR)/unity.o

# Dependency files (generated by -MMD -MP)
DEPS = $(COMMON_OBJS_DEBUG:.o=.d) $(COMMON_OBJS_RELEASE:.o=.d) \
       $(SERVER_OBJ_DEBUG:.o=.d) $(CLIENT_OBJ_DEBUG:.o=.d) \
       $(SERVER_OBJ_RELEASE:.o=.d) $(CLIENT_OBJ_RELEASE:.o=.d) \
       $(TEST_OBJS:.o=.d) $(UNITY_OBJ:.o=.d)

# ============================================================================
# Primary Targets
# ============================================================================

# Default target - build debug versions
.PHONY: all
all: check-tools mbedtls $(BIN_DIR)/sc-server $(BIN_DIR)/sc-client

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
server: mbedtls $(BIN_DIR)/sc-server

.PHONY: client
client: mbedtls $(BIN_DIR)/sc-client

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
		cmake -DCMAKE_INSTALL_PREFIX=$(PWD)/$(DEPS_BUILD_DIR)/$(OS_DIR) -DUSE_SHARED_MBEDTLS_LIBRARY=On $(PWD)/$(DEPS_SRC_DIR)/mbedtls && \
		make -j$$(nproc) && \
		make install && \
		cd $(PWD) && \
		rm -rf $(DEPS_BUILD_DIR)/$(OS_DIR)/build-mbedtls; \
	else \
		echo "mbedTLS already built in $(DEPS_BUILD_DIR)/$(OS_DIR)"; \
	fi

# Build release versions
.PHONY: release
release:
	@echo "Building release for $(OS_ID) ($(OS_DIR))..."
	@$(MAKE) clean
	@$(MAKE) mbedtls
	@$(MAKE) CFLAGS="$(CFLAGS_RELEASE)" $(BIN_DIR)/sc-server-release $(BIN_DIR)/sc-client-release
	@echo "Release build complete for $(OS_ID)"
	@echo "Binaries: $(BIN_DIR)/sc-server-release -> $$(readlink $(BIN_DIR)/sc-server-release)"
	@echo "          $(BIN_DIR)/sc-client-release -> $$(readlink $(BIN_DIR)/sc-client-release)"

# ============================================================================
# Build Rules - Debug
# ============================================================================

# Debug executables
$(BIN_DIR)/sc-server: $(SERVER_OBJS_DEBUG) | $(BIN_DIR)
	$(CC) -o $@ $(SERVER_OBJS_DEBUG) $(LDFLAGS)

$(BIN_DIR)/sc-client: $(CLIENT_OBJS_DEBUG) | $(BIN_DIR)
	$(CC) -o $@ $(CLIENT_OBJS_DEBUG) $(LDFLAGS)

# Debug object files - generic rule
$(OBJ_DIR)/debug/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)/debug
	$(CC) $(CFLAGS_DEBUG) -I$(SRC_DIR) -c -o $@ $<

# ============================================================================
# Build Rules - Release
# ============================================================================

# Release binary versioning helper
define build-release-binary
	@VERSION=$$($(get-version)); \
	$(CC) -o $(BIN_DIR)/$(1)-$$VERSION $(2) $(LDFLAGS); \
	ln -sf $(1)-$$VERSION $(BIN_DIR)/$(1)-release
endef

# Release executables (with versioning)
$(BIN_DIR)/sc-server-release: $(SERVER_OBJS_RELEASE) | $(BIN_DIR)
	$(call build-release-binary,sc-server,$^)

$(BIN_DIR)/sc-client-release: $(CLIENT_OBJS_RELEASE) | $(BIN_DIR)
	$(call build-release-binary,sc-client,$^)

# Release object files - generic rule
$(OBJ_DIR)/release/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)/release
	$(CC) $(CFLAGS_RELEASE) -I$(SRC_DIR) -c -o $@ $<

# ============================================================================
# Test Targets
# ============================================================================

.PHONY: tests
tests: mbedtls $(TEST_BINS)

.PHONY: run-tests
run-tests: mbedtls tests server client
	@for test in $(TEST_BINS); do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done

# Unity framework object
$(UNITY_OBJ): $(TST_DIR)/vendor/unity.c | $(OBJ_DIR)
	$(CC) $(CFLAGS_DEBUG) -c -o $@ $<

# Test executable link command
define link-test
	$(CC) -o $@ $^ $(LDFLAGS)
endef

# Function to get module name from test name
# Default: remove _tests suffix (e.g., message_tests -> message)
# Special case: server_tests depends on dtls module
get-test-module = $(if $(filter server_tests,$(1)),dtls,$(patsubst %_tests,%,$(1)))

# Generic test rule generator
define test-rule
$(BIN_DIR)/sc-$(1): $(OBJ_DIR)/$(1).o $(UNITY_OBJ) $(OBJ_DIR)/debug/$(call get-test-module,$(1)).o | $(BIN_DIR)
	$$(link-test)
endef

# Generate rules for all tests
$(foreach test,$(basename $(notdir $(TEST_SRCS))),$(eval $(call test-rule,$(test))))

# Test object files
$(OBJ_DIR)/%_tests.o: $(TST_DIR)/%_tests.c | $(OBJ_DIR)
	$(CC) $(CFLAGS_DEBUG) -I$(SRC_DIR) -c -o $@ $<

# ============================================================================
# Development Targets
# ============================================================================

# Run targets
.PHONY: run-server
run-server: server | $(DAT_DIR)
	$(BIN_DIR)/sc-server

.PHONY: run-client
run-client: client
	$(BIN_DIR)/sc-client

# Debug with GDB
.PHONY: debug-server
debug-server: server
	gdb $(BIN_DIR)/sc-server

.PHONY: debug-client
debug-client: client
	gdb $(BIN_DIR)/sc-client

# Code formatting
.PHONY: fmt
fmt:
	$(call check-tool,clang-format,Please install clang-format for code formatting)
	@find . -path ./node_modules -prune -o -path ./tests/vendor -prune -o \( -name "*.c" -o -name "*.h" \) -type f -print | while read file; do \
		echo "Formatting: $$file"; \
		clang-format -i "$$file"; \
	done

# Generate self-signed certificates for DTLS
.PHONY: certs
certs:
	$(call check-tool,openssl,Please install OpenSSL for certificate generation)
	@if [ -f "certs/server.key" ] && [ -f "certs/server.crt" ]; then \
		echo "Certificates already exist in certs/ (delete them to regenerate)"; \
	else \
		echo "Generating self-signed certificates..."; \
		mkdir -p certs; \
		openssl req -x509 -newkey rsa:4096 -keyout certs/server.key -out certs/server.crt -sha256 -days 365 -nodes -subj "/CN=localhost"; \
		echo "Certificates generated in certs/"; \
		echo "  - certs/server.key (private key)"; \
		echo "  - certs/server.crt (certificate)"; \
	fi

# Show certificate information
.PHONY: certs-info
certs-info:
	@if [ -f "certs/server.crt" ]; then \
		echo "Certificate information:"; \
		openssl x509 -in certs/server.crt -noout -subject -dates -fingerprint | sed 's/^/  /'; \
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
	@echo "  Amazon: $(amazon_IMAGE) from $(amazon_IMAGE_SOURCE)"
	@echo ""
	@echo "Package Types:"
	@echo "  debian -> $(debian_PACKAGE_TYPE) ($(debian_PACKAGE_CMD))"
	@echo "  amazon -> $(amazon_PACKAGE_TYPE) ($(amazon_PACKAGE_CMD))"

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
install: release
	@echo "Installing Space Captain to $(PREFIX)/bin"
	@install -d $(PREFIX)/bin
	@SERVER_VERSIONED=$$(basename $$(readlink -f $(BIN_DIR)/sc-server-release)) && \
	CLIENT_VERSIONED=$$(basename $$(readlink -f $(BIN_DIR)/sc-client-release)) && \
	echo "Installing server: $$SERVER_VERSIONED" && \
	install -m 755 $(BIN_DIR)/$$SERVER_VERSIONED $(PREFIX)/bin/$$SERVER_VERSIONED && \
	echo "Installing client: $$CLIENT_VERSIONED" && \
	install -m 755 $(BIN_DIR)/$$CLIENT_VERSIONED $(PREFIX)/bin/$$CLIENT_VERSIONED && \
	echo "Creating symlinks..." && \
	ln -sf $$SERVER_VERSIONED $(PREFIX)/bin/space-captain-server && \
	ln -sf $$CLIENT_VERSIONED $(PREFIX)/bin/space-captain-client && \
	echo "Installation complete!"

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: clean-all
clean-all: clean
	rm -rf $(DEPS_DIR) $(PACKAGE_OUT_DIR)

.PHONY: clean-certs
clean-certs:
	@echo "Removing certificates..."
	@rm -f certs/server.key certs/server.crt
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

# Package type mappings for Docker builds
debian_PACKAGE_TYPE = deb
debian_PACKAGE_CMD = package-deb
amazon_PACKAGE_TYPE = rpm
amazon_PACKAGE_CMD = package-rpm

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

# Pattern rule for building Docker images
.PHONY: builder-%
builder-%:
	$(call check-tool,docker,Please install Docker - https://docs.docker.com/get-docker/)
	@echo "Building Space Captain $* builder image..."
	@docker build -f Dockerfile.$* -t space-captain-$*-builder .
	@echo "Builder image created successfully"


# Pattern rule for Docker-based package building
.PHONY: package-%
package-%: clone-mbedtls builder-% certs
	@$(MAKE) package-docker OS=$* PACKAGE_TYPE=$($*_PACKAGE_TYPE) PACKAGE_CMD=$($*_PACKAGE_CMD)

# Common docker packaging target
.PHONY: package-docker
package-docker:
	@echo "Building $(PACKAGE_TYPE) package using Docker..."
	@mkdir -p $(DEPS_BUILD_DIR)/$(OS)
	@if ! docker image inspect space-captain-$(OS)-builder >/dev/null 2>&1; then \
		echo "Error: Builder image 'space-captain-$(OS)-builder' not found. Run 'make builder-$(OS)' first."; \
		exit 1; \
	fi
	@docker run $(DOCKER_RUN_OPTS) space-captain-$(OS)-builder make mbedtls $(PACKAGE_CMD)
	@echo "$(PACKAGE_TYPE) package build complete - check $(PACKAGE_OUT_DIR)/"

# ============================================================================
# Version Management
# ============================================================================

.PHONY: version
version:
	@$(get-version)

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
	@RELEASE=$$(cat .vRELEASE 2>/dev/null || true); \
	if [ -z "$$RELEASE" ]; then RELEASE=0; fi; \
	NEW_RELEASE=$$(($$RELEASE + 1)); \
	echo "$$NEW_RELEASE" > .vRELEASE; \
	echo "Release bumped to $$NEW_RELEASE"

.PHONY: rel-reset
rel-reset:
	@echo "1" > .vRELEASE; \
	echo "Release reset to 1"


# ============================================================================
# Packaging
# ============================================================================

.PHONY: package-deb
package-deb: release certs
	$(check-package-prereqs)
	$(call check-tool,dpkg-deb,Install with: sudo apt-get install dpkg-dev)
	@echo "Building Debian package v$(PACKAGE_VERSION) for $(DEB_ARCH)..."
	@scripts/build-deb-package.sh "$(PACKAGE_VERSION_SAFE)" "$(DEB_ARCH)" "$(BIN_DIR)"
	@RELEASE=$$(cat .vRELEASE 2>/dev/null || echo 1); \
	echo "Debian package created: $(PACKAGE_OUT_DIR)/$(PACKAGE_NAME)_$(PACKAGE_VERSION_SAFE)-$$RELEASE_$(DEB_ARCH).deb"

# RPM package for Amazon Linux / RHEL systems
.PHONY: package-rpm
package-rpm: release certs
	$(check-package-prereqs)
	$(call check-tool,rpmbuild,Install with: sudo yum install rpm-build)
	@echo "Building RPM package v$(PACKAGE_VERSION) for $(RPM_ARCH)..."
	@scripts/build-rpm-package.sh "$(PACKAGE_VERSION_SAFE)" "$(RPM_ARCH)" "$(BIN_DIR)"
	@RELEASE=$$(cat .vRELEASE 2>/dev/null || echo 1); \
	echo "RPM package created: $(PACKAGE_OUT_DIR)/$(PACKAGE_NAME)-$(PACKAGE_VERSION_SAFE)-$$RELEASE.$(RPM_ARCH).rpm"


# ============================================================================
# Help Target
# ============================================================================

.PHONY: help
help:
	@echo "Space Captain Makefile"
	@echo "===================="
	@echo ""
	@echo "Primary Targets:"
	@echo "  make                 Build debug server and client (default)"
	@echo "  make server          Build debug server only"
	@echo "  make client          Build debug client only"
	@echo "  make tests           Build all test executables"
	@echo "  make release         Build optimized release versions"
	@echo ""
	@echo "Development:"
	@echo "  make run-server      Build and run debug server"
	@echo "  make run-client      Build and run debug client"
	@echo "  make run-tests       Build and run all tests"
	@echo "  make debug-server    Debug server with GDB"
	@echo "  make debug-client    Debug client with GDB"
	@echo "  make fmt             Format all code with clang-format"
	@echo ""
	@echo "Maintenance:"
	@echo "  make clean           Remove build artifacts (keeps deps)"
	@echo "  make clean-all       Remove all artifacts including deps"
	@echo "  make clean-certs     Remove generated certificates"
	@echo "  make install         Install release versions to PREFIX"
	@echo "  make package-deb     Build Debian package in $(PACKAGE_OUT_DIR)/"
	@echo "  make package-rpm     Build RPM package in $(PACKAGE_OUT_DIR)/"
	@echo "  make dot             Generate architecture diagram"
	@echo "  make certs           Generate self-signed certificates (idempotent)"
	@echo "  make certs-info      Display certificate information"
	@echo "  make install-tools   Install CLI tools (gemini, claude, codex)"
	@echo "  make update-tools    Update CLI tools to latest versions"
	@echo "  make check-tools     Check for required build tools"
	@echo "  make check-all-tools Check for all optional tools"
	@echo "  make package-info    Display package metadata"
	@echo ""
	@echo "Docker:"
	@echo "  make pull-amazon     Pull Amazon Linux 2023 Docker image"
	@echo "  make pull-debian     Pull Debian slim Docker image"
	@echo "  make builder-<os>    Build Docker builder image (debian/amazon)"
	@echo "  make package-<os>    Build package using Docker (debian/amazon)"
	@echo "  make docker-info     Display Docker configuration"
	@echo ""
	@echo "Dependencies:"
	@echo "  make clone-mbedtls   Clone/update mbedTLS source to $(DEPS_SRC_DIR)"
	@echo "  make mbedtls         Build mbedTLS in $(DEPS_BUILD_DIR)"
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
	@echo ""
	@echo "Environment Variables:"
	@echo "  PREFIX=<path>        Set installation prefix (default: ~/.local)"

# ============================================================================
# Directory Creation
# ============================================================================

$(BIN_DIR) $(DAT_DIR) $(OBJ_DIR)/debug $(OBJ_DIR)/release $(OBJ_DIR):
	mkdir -p $@

# ============================================================================
# Include Dependencies
# ============================================================================

# Include auto-generated dependency files
-include $(DEPS)
