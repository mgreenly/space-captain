# Code Style Guidelines

This document outlines the code style guidelines for the Space Captain project. Adhering to these standards is mandatory for maintaining code quality and consistency.

## 1. General Principles

These high-level rules apply to all code written for this project.

- **Descriptive Naming:** All variables, functions, and types should have clear, descriptive names.
- **No Magic Numbers:** Use `#define` or `enum` for constants instead of embedding literal values directly in the code. This improves readability and maintainability.
- **Single-Line Comments:** Use `//` for all comments. Block comments (`/* ... */`) are not permitted.
- **Document Interfaces:** All public function parameters and return values must be documented.
- **No Trailing Whitespace:** Ensure no lines end with trailing whitespace.

**Example: Magic Numbers**
```c
#define SERVER_MAX_CONNECTIONS 1024

// PREFER:
if (connection_count >= SERVER_MAX_CONNECTIONS) { /* ... */ }

// AVOID:
if (connection_count >= 1024) { /* What does 1024 mean? */ }
```

## 2. Naming Conventions

A consistent naming scheme is enforced for all public APIs and types.

### Type Naming
- **`struct` and `enum` tags:** Use `snake_case`.
- **`typedef`:** Use `snake_case` with a `_t` suffix.

**Example:**
```c
typedef struct sc_client_session {
  int fd;
  // ...
} sc_client_session_t;

typedef enum sc_error_code {
  SC_ERROR_NONE,
  SC_ERROR_FAILED
} sc_error_code_t;
```

### Function Naming
- **Public Functions:** Must follow the pattern `sc_<module>_<function>`.
- **Lifecycle Functions:** Must use `_init` and `_nuke` for creation and destruction.

**Example:**
```c
sc_message_queue_t* sc_message_queue_init(size_t capacity);
void sc_message_queue_nuke(sc_message_queue_t* queue);
```

## 3. API Design and Data Handling

These rules ensure that function signatures are safe, clear, and explicit.

### `const` Correctness
Function parameters that are pointers to data not intended for modification MUST be declared as `const`. This enforces read-only access and clearly communicates intent.

**Example:**
```c
// PREFER: The function cannot modify the content of 'data'.
void sc_auth_verify_token(const char *token);

// AVOID: The function is free to modify 'token'.
void sc_auth_verify_token(char *token);
```

### Pointers to Statically-Sized Arrays
When a function accepts a buffer whose size is fixed at compile time, the parameter MUST be a pointer to an array of that specific size. This embeds the size in the type signature and prevents pointer decay.

**Example:**
```c
// PREFER: Enforces that the buffer is exactly 256 bytes.
void sc_crypto_hash_data(const uint8_t (*data)[256]);

// AVOID: Loses critical size information.
void sc_crypto_hash_data(const uint8_t* data);
```

## 4. Formatting

Code formatting is automated to ensure consistency.

- **Tool:** `clang-format` is used for all C code.
- **Command:** Run `make fmt` after making any changes to apply the project's formatting rules.
