# Command Definitions
- `read FILE`: Load the specified FILE into the prompt context without execution.
- `eval FILE`: Execute the specified FILE explicitly as a prompt.

# Existing Guidelines
# Space Captain Project Guidelines

Space Captain: A toy MMO written in C as a learning experiment for Linux network programming.

## Critical Rules
1. **NO COMPILER WARNINGS** - All code must compile cleanly. Fix warnings immediately.
2. **NO CUSTOM TEST SCRIPTS** - Use `bin/sc-server_tests` for functional testing
3. **ALWAYS USE FORMAT STRINGS** - `log_error("%s", msg)` not `log_error(msg)`
4. **ALWAYS RUN AFTER CHANGES** - `make` to verify builds, `make fmt` for formatting
5. **NEVER COMMIT WITHOUT EXPLICIT REQUEST** - Do not create git commits unless the user explicitly asks you to commit changes. When commits are requested and approved, automatically push to remote.
6. **ALWAYS INCLUDE HIDDEN FILES** - When using any file operation commands (`ls`, `find`, `glob`, etc.), always include hidden files and directories (those starting with `.`). For `ls`, always use the `-a` flag
7. **NO BACKGROUND PROCESSES** - Never run processes in the background. Functional tests are responsible for starting and stopping the applications they require

## Quick Reference

### Build & Run
```bash
make              # Build debug version (DEFAULT)
make release      # Build optimized version
make run-server   # Run server
make run-client   # Run client
make tests        # Build tests
make run-tests    # Build and run all tests
make fmt          # Format code with clang-format
```

### Functional Testing
```bash
bin/sc-server_tests
```

## Architecture Overview

### Network Architecture
- **Server**: Epoll-based event loop, edge-triggered mode, handles 1000s of concurrent connections
- **Protocol**: Fixed 8-byte header (type + length) followed by message body
- **Message Types**: MSG_ECHO (0), MSG_REVERSE (1), MSG_TIME (2)
- **Optimizations**: TCP_NODELAY, connection pooling, adaptive timeouts, EPOLLRDHUP

### Build Structure - Traditional C Compilation
The project uses a **traditional build structure** where each source file compiles to its own object file:

- **Server**: Links `server.o`, `message.o`, and `dtls.o`
- **Client**: Links only `client.o` (self-contained)
- **Tests**: Each test links with Unity framework and required object files
- **Benefits**: Better incremental builds, clearer dependencies, standard C practice
- **Makefile**: Uses `-MMD -MP` flags for automatic dependency tracking
- **Performance**: Clean build ~0.2s, incremental builds ~0.04s per changed file

### File Organization
- **Headers**: message.h, queue.h, dtls.h, server.h, config.h, log.h
- **Sources**: server.c, client.c, queue.c, message.c, dtls.c
- **Tests**: Unity framework in tests/*_tests.c
- **Data**: State files in data/ directory

### Key Components
- `server.c/h` - Main server with epoll event loop
- `client.c` - Test client
- `queue.c/h` - Thread-safe message queue
- `message.c/h` - Protocol definitions and message handling
- `dtls.c/h` - DTLS networking layer with mbedTLS
- `config.h` - All configuration constants
- `log.h` - Logging utilities

## Development Workflow

### Before Making Changes
1. Check requirements in `prd/` folder
2. Understand existing code structure
3. Run `make clean && make` to ensure clean build

### When Modifying PRDs
1. **ALWAYS UPDATE prd/README.md** - When making changes to any PRD document in the `prd/` folder, you must also update the corresponding summary in `prd/README.md`
2. Ensure the summary accurately reflects the key features and changes
3. Keep summaries concise (typically 3-5 bullet points per version)
4. Maintain consistent formatting with other version summaries

### When Modifying Documentation
1. **ALWAYS UPDATE docs/README.md** - When adding or modifying any document in the `docs/` folder, you must also update the table of contents in `docs/README.md`
2. Add new documents to the appropriate category section
3. Use descriptive link text that clearly indicates the document's content
4. Follow the existing format: `- [Link Text](filename.md) - Brief description`
5. Create new category sections if needed for better organization

### After Making Changes
1. Run `make` to check for warnings
2. Run `make fmt` to format code
3. Run functional tests to verify stability
4. Review commit message before approval
5. Commit with descriptive message

### Git Commit Template
Always use this exact commit template:
```
<title>

<body>

co-author: <model>
```

Where `<model>` should be:
- For Claude: `claude-opus-4-20250514`
- For Gemini: `gemini-2.5-pro (via aistudio.google.com)`

**Important**: 
1. Always show the exact commit message to the user BEFORE running `git commit`
2. Wait for user approval of the commit message
3. After creating a commit with user approval, automatically run `git push` to push the changes to the remote repository

## Testing Strategy
- **Unit tests**: Individual component testing (queue_tests.c)
- **Functional tests**: Full system testing (server_tests.c)
- **Manual testing**: Use `make run-server` and `make run-client`
- **Performance testing**: Monitor with 50+ clients

## Common Tasks

### Add New Message Type
1. Add enum value to message.h
2. Add case to message_type_to_string() in message.c
3. Add handler in server.c message processing
4. Update client.c to send new type
5. Add tests in message_tests.c

### Add New Source File
When adding a new source file:
1. Create the .c and .h files in the src/ directory
2. Add `#include "newfile.h"` to files that need the interface
3. Update the Makefile if the new file needs to be linked:
   - Add to `COMMON_SRCS` if used by multiple executables
   - Add to specific `*_OBJS_DEBUG/RELEASE` variables as needed
4. The build system will automatically track dependencies via -MMD -MP flags

### Debug Issues
```bash
make debug-server  # Run server under GDB
make debug-client  # Run client under GDB
```

### Per-File Log Level Configuration
The logging system supports per-file log level configuration. To set a custom log level for a specific source file:

1. Define the desired log level **before** including `log.h`
2. Use one of the predefined constants: `LOG_LEVEL_NONE`, `LOG_LEVEL_FATAL`, `LOG_LEVEL_ERROR`, `LOG_LEVEL_WARN`, `LOG_LEVEL_INFO`, `LOG_LEVEL_DEBUG`

Example:
```c
#define LOG_LEVEL LOG_LEVEL_DEBUG  // Enable debug logging for this file
#include "log.h"
// ... rest of the file
```

Files without an explicit `LOG_LEVEL` definition will use the default level (`LOG_LEVEL_INFO`).

### Version Management
```bash
make major-bump    # Increment major version (0.1.0 -> 1.0.0)
make minor-bump    # Increment minor version (0.1.0 -> 0.2.0)
make patch-bump    # Increment patch version (0.1.0 -> 0.1.1)
make pre-bump      # Increment pre-release version (0.1.0 -> 0.1.0-pre1)
make pre-reset     # Clear pre-release version
make rel-bump      # Increment release number (1 -> 2)
make rel-reset     # Reset release number to 1
```

Package versions follow the pattern:
- RPM: `name-version-release.arch.rpm` (e.g., `space-captain-server-0.1.1-1.x86_64.rpm`)
- DEB: `name_version-release_arch.deb` (e.g., `space-captain-server_0.1.1-1_amd64.deb`)

Note: Pre-release versions use `~` in package filenames (e.g., `space-captain-server-0.1.1~pre1-1.x86_64.rpm`)

## Important Notes
- Server must handle 1000s of concurrent connections at 60 ticks/second
- All networking is non-blocking
- Error handling is critical - check all return values
- Memory management - no leaks allowed (test with valgrind)
- State persistence not yet implemented (v0.1.0 scope)

## Certificate Configuration

The server looks for TLS certificates in the following order:
1. Environment variables: `SC_SERVER_CRT` and `SC_SERVER_KEY`
2. System location: `/etc/space-captain/server.crt` and `/etc/space-captain/server.key`
3. Local directory: `certs/server.crt` and `certs/server.key`

Example usage:
```bash
# Use custom certificate locations
export SC_SERVER_CRT=/path/to/my/cert.pem
export SC_SERVER_KEY=/path/to/my/key.pem
./bin/sc-server

# Use default locations (checks /etc/space-captain/ then certs/)
./bin/sc-server
```


@docs/code-style.md
