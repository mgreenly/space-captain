# Code Style Guidelines

This document outlines the code style requirements for the Space Captain project.

## General Style Requirements

- Single-line comments only (`//` not `/* */`)
- Document all function parameters and return values
- Trim trailing whitespace
- Use consistent indentation (clang-format handles this)
- Descriptive variable and function names

## Function Naming Convention

All public functions must use the pattern `<project>_<module>_<function>` where:
- `<project>` is the project prefix `sc` for (Space Captain)
- `<module>` matches the source filename (e.g., `queue` for queue.c)
- `<function>` is the descriptive function name

### Lifecycle Functions

Module creation/destruction functions must use `*_init` and `*_nuke` (not `*_create`/`*_destroy`)

### Examples

- `sc_queue_init()`, `sc_queue_nuke()`
- `sc_worker_pool_init()`, `sc_worker_pool_nuke()`
- `sc_state_write()`, `sc_state_load()`, `sc_state_nuke()`

## Formatting

Always run `make fmt` after making changes to ensure consistent formatting across the codebase. This uses clang-format with the project's configuration.