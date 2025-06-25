# CLAUDE.md
This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Space Captain is a learning project for building an online multiplayer game in C, described as a blend of Eve Online and Dwarf Fortress with Factorio elements, presented via a text-based interface. The project focuses on learning C and Linux programming.

## Build System & Commands

The project uses a `makefile` build system.

**Core Build Commands:**
- `make` or `make all` - Build both server and client with debug flags
- `make release` - Build optimized release versions
- `make clean` - Remove all build artifacts and tags
- `make install` - Install binaries to `$PREFIX` (defaults to `$HOME/.local/bin`)

**Testing:**
- `make test` - Build and run all unit tests
- `make tests` - Build test binaries only
- Individual test files follow pattern: `tst/*_tests.c` → `bin/*_tests`

**Development:**
- `make tags` - Generate ctags for navigation
- `make run-server` - Run server with `dat/state.dat dat/dump.dat`
- `make run-client` - Run client
- `make debug-server` - Run server in GDB with TUI
- `make debug-client` - Run client in GDB with TUI
- `make fmt` - Apply GNU indent formatting to all C source and header files (excluding vendor/)

## Architecture

**Core Components:**
- **Server** (`src/server.c`) - Main server process with threading support, manages game state and client connections
- **Client** (`src/client.c`) - Simple client that demonstrates message queue usage
- **Message System** (`src/message.h`, `src/queue.h`) - Thread-safe message queue for client-server communication
- **Game State** (`src/state.h`, `src/game.h`) - Game state management and core game logic
- **Network Layer** (`src/network.h`) - Network communication handling
- **Configuration** (`src/config.h`) - Configuration loading and management

**Key Patterns:**
- Server includes all `.c` files directly in `server.c` (unusual pattern)
- Client includes `queue.c` directly
- Thread-safe message queue using pthread mutex/condition variables
- State files stored in `dat/` directory
- Unity testing framework used for unit tests

**Message Types:**
- `MSG_ECHO` - Simple echo message
- `MSG_REVERSE` - Reverse string message  
- `MSG_TIME` - Time-related message

## Dependencies

**Required:** gcc/clang, pkg-config
**Optional:** valgrind, ctags

## Testing Framework

Uses Unity testing framework. Test files follow naming convention `*_tests.c` in the `tst/` directory.

## Code Style Guidelines

- Always trim trailing whitespace when modifying *.c files
- Always generate function comments that describe the parameters and return values
- Alawys use single line comments and never use block comments.
- Always apply GNU indent to any C source or header files after modifying them by running `make fmt` or using the indent command directly

## Git Commit Guidelines

- When creating commits, use a single line co-author attribution:
    ```
    Co-authored-by: Claude <noreply@anthropic.com>
    ```


