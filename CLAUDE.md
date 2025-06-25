# CLAUDE.md

Space Captain: A toy MMO written in "C" as a learning experiment.

## Build Commands
- `make` / `make all` - Build with debug flags
- `make release` - Build optimized versions
- `make clean` - Remove build artifacts
- `make install` - Install to `$PREFIX` (default: `$HOME/.local/bin`)
- `make test` - Build and run tests
- `make fmt` - Format all *.c/*.h files
- `make run-server` / `make run-client` - Run binaries
- `make debug-server` / `make debug-client` - Debug with GDB

## Architecture
**Files:** Server includes all `.c` files directly in `server.c`. Client includes `queue.c`.
**Components:** server.c, client.c, message.h, queue.h, state.h, game.h, network.h, config.h
**Storage:** State files in `dat/` directory
**Testing:** Unity framework, tests in `tst/*_tests.c`

## Style Rules
- Trim trailing whitespace
- Document function parameters/returns
- Use single-line comments only
- Run `clang-format -i` after modifications
