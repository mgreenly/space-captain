# CLAUDE.md

Space Captain: A toy MMO written in "C" as a learning experiment.

**Requirements:** Project requirements are specified in the `req/` folder.

## Build Commands
- `make` / `make all` - Build with debug flags
- `make release` - Build optimized versions
- `make clean` - Remove build artifacts
- `make install` - Install to `$PREFIX` (default: `$HOME/.local/bin`)
- `make test` - Build and run tests
- `make tests` - Build test binaries
- `make fmt` - Format all *.c/*.h files
- `make run-server` / `make run-client` - Run binaries
- `make debug-server` / `make debug-client` - Debug with GDB
- `make bump-buildnumber` - Increment build number
- `make bump-patch` - Increment patch version
- `make bump-minor` - Increment minor version
- `make bump-major` - Increment major version

## Architecture
**Files:** Server includes all `.c` files directly in `server.c`. Client includes `queue.c`.
**Components:** server.c, client.c, message.h, queue.c/h, state.c/h, game.c/h, network.c/h, config.c/h, log.h
**Storage:** State files in `dat/` directory
**Testing:** Unity framework, tests in `tst/*_tests.c`

## Style Rules
- Trim trailing whitespace
- Document function parameters/returns
- Use single-line comments only
- Run `clang-format -i` after modifications
- Always use format strings with log macros: `log_error("%s", "message")` not `log_error("message")`
