# CLAUDE.md - Space Captain Project Guidelines

Space Captain: A toy MMO written in C as a learning experiment for Linux network programming.

## Critical Rules
1. **NO COMPILER WARNINGS** - All code must compile cleanly. Fix warnings immediately.
2. **NO CUSTOM TEST SCRIPTS** - Use `bin/server_client_tests` for functional testing
3. **ALWAYS USE FORMAT STRINGS** - `log_error("%s", msg)` not `log_error(msg)`
4. **ALWAYS RUN AFTER CHANGES** - `make` to verify builds, `make fmt` for formatting
5. **NEVER COMMIT WITHOUT EXPLICIT REQUEST** - Do not create git commits unless the user explicitly asks you to commit changes. When commits are requested and approved, automatically push to remote.
6. **ALWAYS INCLUDE HIDDEN FILES** - When using any file operation commands (`ls`, `find`, `glob`, etc.), always include hidden files and directories (those starting with `.`). For `ls`, always use the `-a` flag

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
# Quick test (5 clients, 5 seconds)
TEST_NUM_CLIENTS=5 TEST_RUNTIME_SECONDS=5 bin/server_client_tests

# Standard test (10 clients, 5 seconds - default)
bin/server_client_tests

# Stress test (50 clients, 30 seconds)
TEST_NUM_CLIENTS=50 TEST_RUNTIME_SECONDS=30 bin/server_client_tests
```

## Architecture Overview

### Network Architecture
- **Server**: Epoll-based event loop, edge-triggered mode, handles 5000+ concurrent connections
- **Protocol**: Fixed 8-byte header (type + length) followed by message body
- **Message Types**: MSG_ECHO (0), MSG_REVERSE (1), MSG_TIME (2)
- **Optimizations**: TCP_NODELAY, connection pooling, adaptive timeouts, EPOLLRDHUP

### Build Structure - Unified C Compilation
The project uses a **unified build structure** where source files are included directly into the main compilation unit:

- **Server (`server.c`)**: Includes `queue.c` and `worker.c` after headers but before implementation
- **Client (`client.c`)**: Self-contained, no additional source includes needed
- **Tests**: Each test file includes its dependencies directly (e.g., `queue_tests.c` includes `queue.c`)
- **Benefits**: Faster compilation, better optimization, simpler dependency management
- **Makefile**: Uses `-MMD -MP` flags for automatic dependency tracking
- **Include Order**: System headers → Project headers → Implementation includes (.c files) → Main implementation

### File Organization
- **Headers**: message.h, queue.h, state.h, game.h, network.h, config.h, log.h
- **Sources**: server.c, client.c, worker.c, queue.c
- **Tests**: Unity framework in tests/*_tests.c
- **Data**: State files in data/ directory

### Key Components
- `server.c` - Main server with epoll event loop
- `client.c` - Test client
- `worker.c/h` - Worker thread pool for message processing
- `queue.c/h` - Thread-safe message queue
- `config.h` - All configuration constants
- `message.h` - Protocol definitions

## Development Workflow

### Before Making Changes
1. Check requirements in `prds/` folder
2. Understand existing code structure
3. Run `make clean && make` to ensure clean build

### When Modifying PRDs
1. **ALWAYS UPDATE prds/README.md** - When making changes to any PRD document in the `prds/` folder, you must also update the corresponding summary in `prds/README.md`
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

**Important**: 
1. Always show the exact commit message to the user BEFORE running `git commit`
2. Wait for user approval of the commit message
3. After creating a commit with user approval, automatically run `git push` to push the changes to the remote repository

## Testing Strategy
- **Unit tests**: Individual component testing (queue_tests.c)
- **Functional tests**: Full system testing (server_client_tests.c)
- **Manual testing**: Use `make run-server` and `make run-client`
- **Performance testing**: Monitor with 50+ clients

## Common Tasks

### Add New Message Type
1. Add enum value to message.h
2. Add case to message_type_to_string()
3. Add handler in worker.c
4. Update client.c to send new type
5. Note: Changes to worker.c will be picked up automatically by server.c

### Add New Source File
When adding a new source file that needs to be included:
1. Create the .c and .h files
2. Add `#include "newfile.h"` with other header includes
3. Add `#include "newfile.c"` after all headers but before the main implementation
4. Follow this order: system headers → project headers → implementation includes → main code
5. Update the Makefile only if creating a new executable target
6. The build system will automatically track dependencies via -MMD -MP flags

Example for server.c:
```c
// System headers
#include <stdio.h>
#include <stdlib.h>
// ... other system headers

// Project headers
#include "config.h"
#include "message.h"
#include "queue.h"
#include "worker.h"
#include "newfile.h"  // Add new header here

// Include implementation files
#include "queue.c"
#include "worker.c"
#include "newfile.c"  // Add new implementation here

// Main server implementation starts here
```

### Debug Issues
```bash
make debug-server  # Run server under GDB
make debug-client  # Run client under GDB
```

### Version Management
```bash
make bump-patch    # Increment patch version
make bump-minor    # Increment minor version
make bump-major    # Increment major version
```

## Important Notes
- Server must handle 5000 concurrent connections at 60 ticks/second
- All networking is non-blocking
- Error handling is critical - check all return values
- Memory management - no leaks allowed (test with valgrind)
- State persistence not yet implemented (v0.1.0 scope)


@docs/code-style.md
