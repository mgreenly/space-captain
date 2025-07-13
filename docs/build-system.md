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
