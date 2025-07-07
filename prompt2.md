# Work Plan: Refactor to a Traditional Build System

## 1. Objective

This document outlines the plan to refactor the project's build system from the current "unified build" pattern to a more traditional structure. In the new system, each C source file (`.c`) will be compiled into its own object file (`.o`), and the `Makefile` will be responsible for linking these object files into the final executables. This change will improve modularity, clarify dependencies, and potentially speed up incremental builds.

## 2. High-Level Plan

The refactoring process will be divided into two main phases:

1.  **Source Code Modification:** Remove all `#include "*.c"` directives from the source code. These will be replaced by standard `#include "*.h"` header inclusions.
2.  **Makefile Refactoring:** Update the `Makefile` to discover all source files, define rules for compiling each source file into an object file, and update the linking rules for all executables (server, client, tests) to include the necessary object files.

## 3. Detailed Steps

### Phase 1: Source Code Modification

The core of this phase is to stop including C source files directly.

1.  **Edit `src/server.c`:**
    *   Remove the line `#include "message.c"`.
    *   Remove the line `#include "dtls.c"`.
    *   Ensure that `#include "message.h"` and `#include "dtls.h"` are present.

2.  **Edit `src/client.c`:**
    *   Review the file for any `include "*.c"` statements and remove them. (A quick review suggests it may not have any, but it should be checked).

3.  **Edit Test Files:**
    *   For each file in `tests/*.c` (e.g., `tests/queue_tests.c`, `tests/message_tests.c`):
        *   Remove the include statement for the source file under test (e.g., `#include "../src/queue.c"`).
        *   Ensure the corresponding header file is included (e.g., `#include "queue.h"`).

### Phase 2: Makefile Refactoring

This phase adapts the `Makefile` to handle the new build logic.

1.  **Identify All Source & Object Files:**
    *   Create a variable to hold all common source files from the `src` directory.
    *   Create variables to map these source files to their corresponding object files for both debug and release builds.

    ```makefile
    # In the 'File Definitions' section
    
    # Common source files (excluding main entry points)
    COMMON_SRCS = $(filter-out $(SRC_DIR)/server.c $(SRC_DIR)/client.c, $(wildcard $(SRC_DIR)/*.c))
    
    # Common object files for debug and release builds
    COMMON_OBJS_DEBUG = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/debug/%.o,$(COMMON_SRCS))
    COMMON_OBJS_RELEASE = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/release/%.o,$(COMMON_SRCS))
    ```

2.  **Update Compilation Rules:**
    *   Modify the existing object file rules to be a generic pattern rule that can compile any `.c` file from the `src` directory.

    ```makefile
    # In 'Build Rules - Debug' and 'Build Rules - Release' sections
    
    # Generic rule for debug object files from src
    $(OBJ_DIR)/debug/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)/debug
    	$(CC) $(CFLAGS_DEBUG) -I$(SRC_DIR) -c -o $@ $<
    
    # Generic rule for release object files from src
    $(OBJ_DIR)/release/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)/release
    	$(CC) $(CFLAGS_RELEASE) -I$(SRC_DIR) -c -o $@ $<
    ```

3.  **Update Linking Rules:**
    *   Update the executable targets to link the common object files along with their main object file.

    ```makefile
    # In 'Build Rules - Debug' section
    
    $(BIN_DIR)/server: $(SERVER_OBJ_DEBUG) $(COMMON_OBJS_DEBUG) | $(BIN_DIR)
    	$(CC) -o $@ $^ $(LDFLAGS)
    
    $(BIN_DIR)/client: $(CLIENT_OBJ_DEBUG) $(COMMON_OBJS_DEBUG) | $(BIN_DIR)
    	$(CC) -o $@ $^ $(LDFLAGS)
    
    # In 'Build Rules - Release' section (example for server)
    
    $(BIN_DIR)/server-release: $(SERVER_OBJ_RELEASE) $(COMMON_OBJS_RELEASE) | $(BIN_DIR)
    	@VERSION=$$(cat .VERSION); \
    	# ... (rest of the rule is the same)
    ```

4.  **Update Test Target Rules:**
    *   The test build rules need to be adjusted to link the object file for the code being tested. This is the most complex part, as each test links different dependencies.
    *   We will create a rule for each test executable.

    ```makefile
    # In 'Test Targets' section
    
    # Example for queue_tests
    $(BIN_DIR)/queue_tests: $(OBJ_DIR)/queue_tests.o $(OBJ_DIR)/debug/queue.o | $(BIN_DIR)
    	$(CC) -o $@ $^ $(LDFLAGS)
    
    # Example for message_tests
    $(BIN_DIR)/message_tests: $(OBJ_DIR)/message_tests.o $(OBJ_DIR)/debug/message.o | $(BIN_DIR)
    	$(CC) -o $@ $^ $(LDFLAGS)
    
    # ... and so on for each test.
    ```
    *   The generic rule for test object files remains the same, but the linking rule becomes specific for each test.

5.  **Update Dependency Tracking:**
    *   The `DEPS` variable needs to be updated to include all new object files.

    ```makefile
    # In 'File Definitions' section
    
    ALL_OBJS = $(SERVER_OBJ_DEBUG) $(CLIENT_OBJ_DEBUG) \
               $(SERVER_OBJ_RELEASE) $(CLIENT_OBJ_RELEASE) \
               $(COMMON_OBJS_DEBUG) $(COMMON_OBJS_RELEASE) \
               $(TEST_OBJS)
    
    DEPS = $(ALL_OBJS:.o=.d)
    ```

## 4. Verification

After implementing the changes, the build process must be thoroughly verified to ensure correctness.

1.  **Clean the Build Artifacts:**
    ```bash
    make clean
    ```

2.  **Build Debug Targets:**
    ```bash
    make all
    ```
    *   Verify that `bin/server` and `bin/client` are created successfully.

3.  **Build Release Targets:**
    ```bash
    make release
    ```
    *   Verify that `bin/server-release` and `bin/client-release` are created.

4.  **Build and Run Tests:**
    ```bash
    make run-tests
    ```
    *   Verify that all tests are built and executed successfully, with no test failures.

Completing these verification steps successfully will confirm that the transition to a traditional build system is complete and correct.
