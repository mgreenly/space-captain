# Traditional Build System

This document explains the build system used in the Space Captain project.

## Overview

Space Captain uses a traditional C build system where each source file (`.c`) compiles to its own object file (`.o`), which are then linked together to create executables. This approach provides:

- **Better incremental builds**: Only modified files need recompilation
- **Clearer dependencies**: Explicit linking shows what each executable needs
- **Standard practice**: Follows conventional C project structure
- **Parallel compilation**: Multiple object files can be compiled simultaneously

## Build Performance

- **Clean build**: ~0.2 seconds
- **Incremental build**: ~0.04 seconds per changed file
- **Test suite**: Full test execution in under 10 seconds

## Directory Structure

```
space-captain/
├── src/            # Source files
├── obj/            # Object files (build artifacts)
│   ├── debug/      # Debug build objects
│   └── release/    # Release build objects
├── bin/            # Executables
└── tests/          # Test source files
```

## Makefile Organization

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

Tests link with Unity framework and specific modules:

```makefile
$(BIN_DIR)/queue_tests: $(OBJ_DIR)/queue_tests.o $(UNITY_OBJ) $(OBJ_DIR)/debug/queue.o | $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)
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
- `make` - Build debug server and client
- `make server` - Build debug server only
- `make client` - Build debug client only
- `make release` - Build optimized release versions

### Test Targets
- `make tests` - Build all test executables
- `make run-tests` - Build and run all tests

### Development Targets
- `make clean` - Remove all build artifacts
- `make fmt` - Format code with clang-format

## Compiler Flags

### Common Flags
- `-D_DEFAULT_SOURCE` - Enable POSIX features
- `-std=c18` - Use C18 standard
- `-pedantic -Wall -Wextra` - Enable strict warnings
- `-MMD -MP` - Automatic dependency generation

### Debug vs Release
- Debug: `-O0 -g` - No optimization, debug symbols
- Release: `-O3` - Maximum optimization

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