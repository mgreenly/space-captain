# Traditional Build System

This document explains the build system used in the Space Captain project.

## Overview

Space Captain uses a traditional C build system where each source file (`.c`) compiles to its own object file (`.o`), which are then linked together to create executables. This approach provides:

- **Better incremental builds**: Only modified files need recompilation
- **Clearer dependencies**: Explicit linking shows what each executable needs
- **Standard practice**: Follows conventional C project structure
- **Parallel compilation**: Multiple object files can be compiled simultaneously
- **Tool verification**: Automatic checking for required build tools
- **Cross-platform packaging**: Support for both Debian and RPM packages

## Build Performance

- **Clean build**: ~0.2 seconds
- **Incremental build**: ~0.04 seconds per changed file
- **Test suite**: Full test execution in under 10 seconds

## Directory Structure

```
space-captain/
├── src/              # Source files
├── obj/              # Object files (build artifacts)
│   ├── debug/        # Debug build objects
│   └── release/      # Release build objects
├── bin/              # Executables
├── tests/            # Test source files
├── pkg/              # Package files
│   ├── deb/          # Debian package templates
│   ├── rpm/          # RPM package templates
│   └── out/          # Built packages
├── scripts/          # Build and package scripts
├── .secrets/certs/   # Generated certificates
└── deps/             # Dependencies
    ├── src/          # Third-party source code
    │   └── mbedtls/  # mbedTLS source
    └── build/        # Built dependencies
        ├── debian/   # Debian-specific builds
        └── amazon/   # Amazon Linux builds
```

## Makefile Organization

The Makefile is organized into logical sections for maintainability:

1. **Directory Structure** - Source, build, and output directories
2. **Version and Build** - Version info, build tags, dependencies
3. **Installation Settings** - Install prefix and paths
4. **Compiler Settings** - Compiler flags and options
5. **Package Metadata** - Package information for releases
6. **OS Detection** - Operating system detection and configuration
7. **Architecture Detection** - Architecture mapping for packages
8. **Build Helpers** - Helper functions and macros
9. **File Definitions** - Source and object file lists
10. **Build Targets** - Main build rules
11. **Docker Configuration** - Docker settings and image definitions

### Object File Rules

The Makefile uses pattern rules for building object files:

```makefile
# Debug object files - generic rule
$(OBJ_DIR)/debug/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)/debug
	$(CC) $(CFLAGS_DEBUG) -I$(SRC_DIR) -c -o $@ $<

# Release object files - generic rule
$(OBJ_DIR)/release/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)/release
	$(CC) $(CFLAGS_RELEASE) -I$(SRC_DIR) -c -o $@ $<
```

### Executable Linking

Each executable explicitly lists its required object files:

```makefile
# Server needs server.o, message.o, and dtls.o
SERVER_OBJS_DEBUG = $(SERVER_OBJ_DEBUG) $(OBJ_DIR)/debug/message.o $(OBJ_DIR)/debug/dtls.o

# Client only needs client.o (self-contained)
CLIENT_OBJS_DEBUG = $(CLIENT_OBJ_DEBUG)
```

### Test Linking

Tests are dynamically generated using Make metaprogramming:

```makefile
# Function to get module name from test name
get-test-module = $(if $(filter server_tests,$(1)),dtls,$(patsubst %_tests,%,$(1)))

# Generic test rule generator
define test-rule
$(BIN_DIR)/sc-$(1): $(OBJ_DIR)/$(1).o $(UNITY_OBJ) $(OBJ_DIR)/debug/$(call get-test-module,$(1)).o | $(BIN_DIR)
	$$(link-test)
endef

# Generate rules for all tests
$(foreach test,$(basename $(notdir $(TEST_SRCS))),$(eval $(call test-rule,$(test))))
```

## Dependency Tracking

The build system uses GCC's automatic dependency generation:

- `-MMD`: Generate dependency files
- `-MP`: Add phony targets for headers
- Dependency files are included at the bottom of the Makefile

This ensures that changes to header files trigger recompilation of dependent source files.

## Adding New Source Files

To add a new source file to the project:

1. Create the `.c` and `.h` files in `src/`
2. Update the Makefile to include the new object file:
   ```makefile
   # If used by server
   SERVER_OBJS_DEBUG = $(SERVER_OBJ_DEBUG) $(OBJ_DIR)/debug/message.o \
                       $(OBJ_DIR)/debug/dtls.o $(OBJ_DIR)/debug/newfile.o
   ```
3. The dependency tracking will handle the rest automatically

## Build Targets

### Primary Targets
- `make` - Build debug server and client (with tool checking)
- `make server` - Build debug server only
- `make client` - Build debug client only
- `make release` - Build optimized release versions

### Test Targets
- `make tests` - Build all test executables
- `make run-tests` - Build and run all tests

### Development Targets
- `make clean` - Remove build artifacts (keeps .local)
- `make clean-all` - Remove all artifacts including .local and vendor
- `make clean-certs` - Remove generated certificates
- `make fmt` - Format code with clang-format
- `make certs` - Generate self-signed certificates (idempotent)
- `make certs-info` - Display certificate information

### Package Targets
- `make package-deb` - Build Debian package locally
- `make package-rpm` - Build RPM package locally
- `make package-<os>` - Build package using Docker (debian/amazon)
- `make package-info` - Display package metadata

### Docker Targets
- `make pull-amazon` - Pull Amazon Linux 2023 Docker image
- `make pull-debian` - Pull Debian slim Docker image
- `make builder-<os>` - Build Docker builder image (debian/amazon)
- `make docker-info` - Display Docker configuration

### Information Targets
- `make check-tools` - Check for required build tools
- `make check-all-tools` - Check for all optional tools
- `make arch-info` - Display architecture detection details
- `make version` - Display current version
- `make help` - Show all available targets

## Compiler Flags

### Common Flags
- `-D_DEFAULT_SOURCE` - Enable POSIX features
- `-std=c18` - Use C18 standard
- `-pedantic -Wall -Wextra` - Enable strict warnings
- `-MMD -MP` - Automatic dependency generation

### Debug vs Release
- Debug: `-O0 -g` - No optimization, debug symbols
- Release: `-O3` - Maximum optimization

### External Dependencies vs Project Code

The build system uses different compiler flags for external dependencies (like mbedTLS and Unity) compared to project code:

#### Project Code Flags
Project code uses strict warning flags with `-Werror` to ensure high code quality:
```makefile
CFLAGS_BASE = -D_DEFAULT_SOURCE -D_FORTIFY_SOURCE=2 -std=c18 -pedantic \
              -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wwrite-strings \
              -Werror -Wall -Wextra -Wformat=2 -Wconversion -Wcast-qual -Wundef
```

Additional compiler-specific flags:
- **GCC**: `-fanalyzer -fstack-clash-protection -Wduplicated-cond -Wduplicated-branches`
- **Clang**: `-fcolor-diagnostics -Wno-gnu-zero-variadic-macro-arguments`

#### External Dependency Flags
External dependencies use minimal, functional flags since we don't control their code:
```makefile
EXTERNAL_DEPS_CFLAGS = -O2 -fPIC -D_DEFAULT_SOURCE -Wno-error
```

This approach:
- Avoids build failures from warnings in third-party code
- Maintains optimization (`-O2`) for performance
- Ensures position-independent code (`-fPIC`) for shared libraries
- Explicitly disables treating warnings as errors (`-Wno-error`)

For mbedTLS specifically, we also pass:
- `-DMBEDTLS_FATAL_WARNINGS=OFF` to CMake to disable its internal warning-as-error settings

## Tool Checking

The build system automatically checks for required tools:

```makefile
# Check for a specific tool
$(call check-tool,gcc,Please install GCC compiler)

# Required tools are checked on every build
make check-tools    # Check essential tools
make check-all-tools # Check all optional tools
```

Tools checked include:
- **Required**: gcc, make, git
- **Optional**: cmake, docker, aws, dpkg-deb, rpmbuild, clang-format, openssl, npm, dot, gdb

## Architecture Detection

The build system automatically detects and maps architectures:

```makefile
# Machine architecture detection
MACHINE_ARCH := $(shell uname -m)

# Architecture mappings
x86_64  → Debian: amd64,    RPM: x86_64
aarch64 → Debian: arm64,    RPM: aarch64
armv7l  → Debian: armhf,    RPM: armv7hl
```

Use `make arch-info` to see current architecture detection.

## Package Building

The build system supports multiple package formats:

### Local Package Building
```bash
make package-deb    # Build Debian package
make package-rpm    # Build RPM package
```

### Docker-based Package Building
```bash
make package-debian  # Build in Debian container
make package-amazon  # Build in Amazon Linux container
```

Packages are output to `pkg/out/` with proper architecture naming.

## Certificate Management

Self-signed certificates for DTLS:

```bash
make certs          # Generate certificates (idempotent)
make certs-info     # Show certificate details
make clean-certs    # Remove certificates
```

Certificates are only generated if they don't already exist.

## Pattern Rules

The Makefile uses pattern rules to reduce duplication:

- `builder-%` - Build any Docker builder image
- `package-%` - Build packages using Docker
- Object file compilation for debug/release
- Test executable generation

## Troubleshooting

### Missing Symbols
If you get undefined reference errors, ensure:
1. The source file is compiled to an object file
2. The object file is included in the executable's dependencies
3. Header files have proper include guards

### Incremental Build Issues
If incremental builds aren't working:
1. Check that dependency files (`.d`) are being generated
2. Ensure the `-include $(DEPS)` line is present in the Makefile
3. Try `make clean && make` to rebuild from scratch

### Tool Not Found
If a tool is missing:
1. Run `make check-all-tools` to see what's missing
2. Install the missing tool using the provided instructions
3. For Docker builds, ensure Docker daemon is running

### Package Build Failures
If package building fails:
1. Ensure you've run `make release` first
2. Check architecture with `make arch-info`
3. Verify package metadata with `make package-info`
4. For Docker builds, ensure builder image exists

## Adding Support for a New OS

To add support for a new operating system to the build system, follow these steps:

### 1. Create Docker Builder Image

Create `Dockerfile.<os>` in the project root:
```dockerfile
FROM <base-image>

# Install build dependencies
RUN <package-manager> install -y \
    gcc \
    make \
    cmake \
    python3 \
    <package-builder> \    # dpkg-dev or rpm-build
    chrpath \               # For RPATH stripping
    gosu \                  # For user switching
    git \
    clang-tools-extra \
    graphviz \
    gdb \
    openssl \
    openssl-devel \         # or libssl-dev for Debian-based
    && <cleanup-command>    # yum/dnf clean all or rm -rf /var/lib/apt/lists/*

# Install gosu for proper user switching
RUN curl -L https://github.com/tianon/gosu/releases/download/1.17/gosu-amd64 -o /usr/local/bin/gosu && \
    chmod +x /usr/local/bin/gosu

# Copy entrypoint script
COPY scripts/docker-entrypoint-<os>.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["/bin/bash"]
```

### 2. Create Docker Entrypoint Script

Create `scripts/docker-entrypoint-<os>.sh`:
```bash
#!/bin/bash
# Docker entrypoint script for <os> builder container

USER_ID=${USER_ID:-1000}
GROUP_ID=${GROUP_ID:-1000}

# Create group and user with matching IDs (silently)
groupadd -g $GROUP_ID -o builduser 2>/dev/null || true
useradd -u $USER_ID -g $GROUP_ID -o -m -s /bin/bash builduser 2>/dev/null || true

# Get OS ID from /etc/os-release
OS_ID=$(grep '^ID=' /etc/os-release | cut -d= -f2 | tr -d '"')

# Set custom PS1 for builduser
echo "export PS1='\\[\\e[36;1;2m\\]${OS_ID}@\\W\\[\\e[0m\\]\\\$ '" >> /home/builduser/.bashrc

# Change to workspace directory
cd /workspace

# Use gosu to properly switch user with TTY preservation
exec gosu builduser "$@"
```

### 3. Update Makefile OS Detection

Add OS detection in the Makefile (around line 255):
```makefile
else ifeq ($(OS_ID),<os>)
    OS_DIR := <os>
    PACKAGE_TYPE := <deb|rpm>
    PACKAGE_BUILDER := <dpkg-deb|rpmbuild>
```

Add to the error message at the end of the OS detection block:
```makefile
$(error Unsupported OS: $(OS_ID). This project only supports Debian (ID=debian), Ubuntu (ID=ubuntu), Amazon Linux (ID=amzn), Fedora (ID=fedora) and <OS> (ID=<os>))
```

### 4. Update Architecture Mapping

Add architecture mapping (around line 297):
```makefile
else ifeq ($(OS_ID),<os>)
    PACKAGE_ARCH := $(DEB_ARCH)  # or $(RPM_ARCH) depending on package type
```

### 5. Update Docker Configuration

Add Docker image configuration (around line 805):
```makefile
<os>_IMAGE = <base-image>
<os>_IMAGE_SOURCE = <Docker Hub|Other Registry>
```

### 6. Add Build Targets

The following targets will be automatically available due to pattern rules:
- `make pull-<os>` - Pull base Docker image
- `make build-<os>` - Build Docker builder image
- `make shell-<os>` - Start interactive shell in builder
- `make package-<os>` - Build package using Docker

You need to manually add:

1. Update the `packages` target to include the new OS (around line 1026):
```makefile
echo "=== Building <OS> package ==="; \
$(MAKE) package-<os>; \
echo ""; \
```

2. Update help text sections:
- Docker section: Add to the list of supported OSes
- Packaging section: Add to the list of supported OSes

### 7. Create Package Templates

Create the directory structure:
```bash
mkdir -p pkg/<os>/templates
```

For DEB-based systems, create:
- `pkg/<os>/templates/control.template`
- `pkg/<os>/templates/postinst`
- `pkg/<os>/templates/prerm`
- `pkg/<os>/templates/conffiles`
- `pkg/<os>/templates/space-captain-server.service`

For RPM-based systems, create:
- `pkg/<os>/templates/space-captain.spec.template`
- `pkg/<os>/templates/space-captain-server.service`

### 8. Test the New OS Support

```bash
# Build Docker image
make build-<os>

# Enter build environment
make shell-<os>

# Inside container, test building
make clean-all
make
make tests
make run-tests

# Test packaging
make package

# From host, test full package build
make package-<os>
```

### Example: Adding Rocky Linux Support

Here's a concrete example of adding Rocky Linux support:

1. Create `Dockerfile.rocky`:
```dockerfile
FROM rockylinux:9

RUN dnf install -y \
    gcc make cmake python3 rpm-build chrpath \
    tar gzip shadow-utils util-linux \
    git clang-tools-extra graphviz gdb \
    openssl openssl-devel \
    && dnf clean all

RUN curl -L https://github.com/tianon/gosu/releases/download/1.17/gosu-amd64 -o /usr/local/bin/gosu && \
    chmod +x /usr/local/bin/gosu

COPY scripts/docker-entrypoint-rocky.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["/bin/bash"]
```

2. Copy and adapt the entrypoint script:
```bash
cp scripts/docker-entrypoint-fedora.sh scripts/docker-entrypoint-rocky.sh
```

3. Update Makefile OS detection:
```makefile
else ifeq ($(OS_ID),rocky)
    OS_DIR := rocky
    PACKAGE_TYPE := rpm
    PACKAGE_BUILDER := rpmbuild
```

4. Add remaining configuration and test.
