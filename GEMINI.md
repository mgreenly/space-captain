# GEMINI.md - Space Captain Project Guidelines

## 1. Project Overview

Space Captain is a client-server MMO project built in C as a technical exploration of Linux network programming, systems optimization, and game design. The long-term vision is a game that blends elements of Eve Online, Dwarf Fortress, and Factorio.

The project evolves from a foundational technical demo (v0.1.0) with a CLI client and robust server infrastructure, through platform-specific graphical clients (v0.2.0-0.4.0), to a persistent world with advanced game systems (v0.5.0+).

## 2. Critical Rules
1.  **NO COMPILER WARNINGS**: All code must compile cleanly. Fix warnings immediately.
2.  **NO CUSTOM TEST SCRIPTS**: Use `bin/server_client_tests` for functional testing.
3.  **ALWAYS USE FORMAT STRINGS**: Use `log_error("%s", msg)`, not `log_error(msg)`.
4.  **ALWAYS RUN AFTER CHANGES**: Run `make` to verify builds and `make fmt` for formatting.
5.  **NEVER COMMIT WITHOUT EXPLICIT REQUEST**: Do not create git commits unless explicitly asked. When approved, I will automatically push to the remote.
6.  **ALWAYS INCLUDE HIDDEN FILES**: When using file operation commands (`ls`, `find`, `glob`), always include hidden files (e.g., `ls -a`).

## 3. Development Plan Summary (from prds/)

The project follows a quarterly release cycle. Future plans are speculative.

*   **v0.1.0 (Active)**: Core infrastructure, reliable-UDP, server-authoritative architecture, and a CLI/ncurses client for 2D space combat.
*   **v0.2.0 (Planned)**: TLS security, OIDC authentication, and cross-platform CLI support.
*   **v0.3.0 (Planned)**: Linux graphical client with Vulkan.
*   **v0.4.0 (Planned)**: macOS graphical client with Metal.
*   **v0.5.0 (Planned)**: Windows graphical client with DirectX.
*   **v0.6.0 (Planned)**: Foundations for a persistent world, including crew, economy, and manufacturing.

## 4. Quick Reference

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
# Standard test (10 clients, 5 seconds)
bin/server_client_tests

# Quick test (5 clients, 5 seconds)
TEST_NUM_CLIENTS=5 TEST_RUNTIME_SECONDS=5 bin/server_client_tests

# Stress test (50 clients, 30 seconds)
TEST_NUM_CLIENTS=50 TEST_RUNTIME_SECONDS=30 bin/server_client_tests
```

## 5. Architecture & Technical Documentation

### Core Architecture
*   **Network**: Epoll-based event loop (edge-triggered), handling 5000+ connections.
*   **Protocol**: Fixed 8-byte header (type + length) + message body.
*   **Build**: Unified C compilation (`.c` files are included directly into main compilation units like `server.c`). The Makefile uses `-MMD -MP` for automatic dependency tracking.
*   **Key Components**: `server.c` (main loop), `client.c` (test client), `worker.c`/`h` (thread pool), `queue.c`/`h` (thread-safe MPMC queue), `config.h` (constants), `message.h` (protocol).

### Documentation Index (from docs/)
I am aware of the detailed documentation available in the `docs/` directory, covering:
*   **Architecture**: Diagrams, main loop, MPMC queue, worker thread pool.
*   **Build System**: The unified C project structure.
*   **Development Guidelines**: Code style and naming conventions.
*   **Game Design**: Coordinate precision and the speed table.
*   **Research**: MMO architecture, state management, security (UDP, TLS, OIDC), and UI (NCurses).

## 6. Development Workflow

### Before Making Changes
1.  Review requirements in the relevant `prds/version-X.Y.Z.md` file.
2.  Understand the existing code structure and consult `docs/` if needed.
3.  Run `make clean && make` to ensure a clean build.

### Modifying Documentation or PRDs
*   **When modifying `docs/`**: I MUST update the table of contents in `docs/README.md`.
*   **When modifying `prds/`**: I MUST update the corresponding summary in `prds/README.md`.

### After Making Changes
1.  Run `make` to check for warnings.
2.  Run `make fmt` to format code.
3.  Run functional tests (`make run-tests`) to verify stability.
4.  Propose a descriptive commit message for your approval.

### Git Commit Template
I will use this exact commit template:
```
<title>

<body>

co-author: gemini-2.5-pro (via aistudio.google.com)
```
**Important**: I will always show the exact commit message for approval before running `git commit`. After a commit is approved, I will automatically run `git push`.

## 7. Common Tasks

### Adding a New Message Type
1.  Add the enum value to `message.h`.
2.  Add the case to `message_type_to_string()` in `message.c`.
3.  Add the handler in `worker.c`.
4.  Update `client.c` to send the new message type.

### Adding a New Source File
1.  Create the `.c` and `.h` files.
2.  In the main compilation unit (e.g., `server.c`), add `#include "newfile.h"` with other project headers.
3.  Add `#include "newfile.c"` after all headers but before the main implementation.
4.  The build system will track dependencies automatically.