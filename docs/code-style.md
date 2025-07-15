# Code Style Guidelines

This document outlines the code style guidelines for the Space Captain project.

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

## Pointers to Statically-Sized Arrays

When a function accepts a buffer whose size is fixed at compile time, the parameter MUST be declared as a pointer to an array of that specific size.

**Rationale:**

*   **Type Safety:** Enforces that only arrays of the exact specified size can be passed, preventing common buffer size errors at compile time.
*   **Clarity:** The function's signature explicitly documents the expected buffer size.
*   **Prevents Pointer Decay:** It distinguishes between a pointer to a single element and a pointer to an entire array.

**Example:**

```c
// PREFER: The compiler will enforce that the argument is an array of 256 bytes.
void handle_message(uint8_t (*message)[256]);

// AVOID: Any pointer to a uint8_t can be passed, losing size information.
void handle_message(uint8_t* message);
```
