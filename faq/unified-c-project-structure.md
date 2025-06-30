# Unified C Project Structure

## What is a Unified C Project Structure?

A unified C project structure is a build approach where source files (`.c` files) are included directly into a single compilation unit rather than being compiled separately and linked together. This is sometimes called "single compilation unit" or "unity build".

## How It Works

Instead of the traditional approach:
```bash
# Traditional: compile each file separately
gcc -c main.c -o main.o
gcc -c module1.c -o module1.o
gcc -c module2.c -o module2.o
gcc main.o module1.o module2.o -o program
```

The unified approach uses:
```bash
# Unified: compile everything as one unit
gcc main.c -o program
```

Where `main.c` includes other `.c` files directly:
```c
// main.c
#include <stdio.h>
#include "module1.h"
#include "module2.h"

// Include implementation files
#include "module1.c"
#include "module2.c"

int main() {
    // ...
}
```

## Benefits

1. **Faster Compilation**: The compiler only needs to parse headers once, reducing overall compilation time for full builds.

2. **Better Optimization**: The compiler can see all code at once, enabling more aggressive optimizations like cross-file inlining.

3. **Simpler Build System**: No need to track object files or manage complex linking rules.

4. **Automatic Dependency Management**: When using compiler flags like `-MMD -MP`, all dependencies are tracked in a single `.d` file.

5. **Reduced Binary Size**: Better dead code elimination since the compiler can see all code paths.

## Drawbacks

1. **Slower Incremental Builds**: Changing any file requires recompiling everything.

2. **Namespace Pollution**: All static variables and functions become visible across all included files.

3. **Memory Usage**: The compiler needs more memory to process larger compilation units.

## Implementation in Space Captain

### Project Structure

The Space Captain project uses a unified build structure for its main executables:

- **Server (`src/server.c`)**: Includes `queue.c` and `worker.c`
- **Client (`src/client.c`)**: Self-contained (no additional includes needed)
- **Tests**: Each test file includes its dependencies (e.g., `queue_tests.c` includes `queue.c`)

### Include Order

The project follows a strict include order:

1. System headers (`<stdio.h>`, `<stdlib.h>`, etc.)
2. Project headers (`"config.h"`, `"message.h"`, etc.)
3. Implementation includes (`"queue.c"`, `"worker.c"`)
4. Main implementation code

Example from `server.c`:
```c
// System headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// ... more system headers

// Project headers
#include "config.h"
#include "message.h"
#include "queue.h"
#include "worker.h"
#include "log.h"

// Include implementation files
#include "queue.c"
#include "worker.c"

// Server implementation starts here
typedef struct client_buffer {
    // ...
}
```

### Makefile Configuration

The Makefile is simplified since each executable has only one object file:

```makefile
# Object files for debug builds
SERVER_OBJ_DEBUG = $(OBJ_DIR)/debug/server.o
CLIENT_OBJ_DEBUG = $(OBJ_DIR)/debug/client.o

# Compilation rule
$(OBJ_DIR)/debug/server.o: $(SERVER_MAIN) | $(OBJ_DIR)/debug
    $(CC) $(CFLAGS_DEBUG) -I$(SRC_DIR) -c -o $@ $<
```

The `-MMD -MP` flags automatically generate dependency files that track all included files:

```makefile
# server.d will contain dependencies on server.c, queue.c, worker.c, and all headers
-include $(DEPS)
```

### Best Practices for This Structure

1. **Header Guards**: Always use header guards to prevent multiple inclusion:
   ```c
   #ifndef QUEUE_H
   #define QUEUE_H
   // declarations
   #endif
   ```

2. **Static Functions**: Use `static` for file-local functions to avoid naming conflicts:
   ```c
   static void helper_function() {
       // ...
   }
   ```

3. **Clear Separation**: Keep declarations in `.h` files and definitions in `.c` files.

4. **Documentation**: Document which files are included where (see CLAUDE.md).

## When to Use This Structure

The unified build structure works well for:

- Small to medium-sized projects
- Projects where build simplicity is valued
- Embedded systems with limited resources
- Projects that benefit from aggressive optimization

It may not be suitable for:

- Very large projects (100+ files)
- Projects requiring fast incremental builds
- Libraries that need clear API boundaries
- Projects with multiple developers working on different modules

## Adding New Source Files

To add a new source file to the unified build:

1. Create the `.c` and `.h` files
2. Add `#include "newfile.h"` with other headers in the target file
3. Add `#include "newfile.c"` after headers but before the main implementation
4. No Makefile changes needed (dependencies are automatic)

Example:
```c
// In server.c, adding a new stats module
#include "stats.h"      // Add with other headers
// ...
#include "stats.c"      // Add with other .c includes
```

## Conclusion

The unified C project structure in Space Captain provides a simple, efficient build system that maximizes compiler optimizations while minimizing build complexity. It's particularly well-suited for this project's size and structure, though developers should be aware of its trade-offs when considering it for other projects.