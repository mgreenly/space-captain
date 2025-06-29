# CLAUDE.md - Space Captain Project Guidelines

Space Captain: A toy MMO written in C as a learning experiment for Linux network programming.

## Critical Rules
1. **NO COMPILER WARNINGS** - All code must compile cleanly. Fix warnings immediately.
2. **NO CUSTOM TEST SCRIPTS** - Use `bin/server_client_tests` for functional testing
3. **ALWAYS USE FORMAT STRINGS** - `log_error("%s", msg)` not `log_error(msg)`
4. **ALWAYS RUN AFTER CHANGES** - `make` to verify builds, `make fmt` for formatting

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

### File Organization
- **Server compilation**: Includes all .c files directly in server.c (no separate compilation)
- **Client compilation**: Only includes queue.c
- **Headers**: message.h, queue.h, state.h, game.h, network.h, config.h, log.h
- **Tests**: Unity framework in tst/*_tests.c
- **Data**: State files in dat/ directory

### Key Components
- `server.c` - Main server with epoll event loop
- `client.c` - Test client
- `worker.c/h` - Worker thread pool for message processing
- `queue.c/h` - Thread-safe message queue
- `config.h` - All configuration constants
- `message.h` - Protocol definitions

## Development Workflow

### Before Making Changes
1. Check requirements in `req/` folder
2. Understand existing code structure
3. Run `make clean && make` to ensure clean build

### After Making Changes
1. Run `make` to check for warnings
2. Run `make fmt` to format code
3. Run functional tests to verify stability
4. Commit with descriptive message

### Code Style Requirements
- Single-line comments only (`//` not `/* */`)
- Document all function parameters and return values
- Trim trailing whitespace
- Use consistent indentation (clang-format handles this)
- Descriptive variable and function names

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