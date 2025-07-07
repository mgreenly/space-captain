# Work Plan: Refactoring to Traditional Build System (Enhanced Version)

## 1. Objective

The goal of this document is to guide a safe, incremental refactoring from the current unified build into a more standard, modular build system. Each C source file (`.c`) will compile into an object file (`.o`) directly, and these object files will link together via explicit Makefile rules. This will enhance maintainability, improve clarity of file dependencies, and may improve incremental build performance.

## 2. High-Level Strategy

To minimize risk and ensure smooth integration, this process emphasizes incremental changes, thorough documentation, performance tracking, rollback readiness, and explicit dependency management. The refactor workflow broadly splits into two main phases:

1. **Phase 1 (Source Code Modification):**
    - Remove `#include "*.c"` statements.
    - Ensure using standard header inclusions (`*.h`) exclusively.
    - Verify proper use of header guards.

2. **Phase 2 (Makefile Adaptation):**
    - Clearly identify and handle all source/object files.
    - Define generic compilation rules for object files.
    - Adjust linking explicitly for executables and tests.
    - Maintain correct dependency flags (`-MMD -MP`).

## 3. Directory Structure Clarification

After refactoring, the project should resemble this explicit structure:

```
space-captain/
├── src/
│   ├── server.c
│   ├── client.c
│   ├── message.c
│   ├── message.h
│   ├── dtls.c
│   ├── dtls.h
│   ├── queue.c
│   ├── queue.h
│   └── (additional component files...)
├── tests/
│   ├── queue_tests.c
│   ├── message_tests.c
│   └── (additional tests...)
├── obj/
│   ├── debug/
│   └── release/
├── bin/
├── docs/
├── prd/
└── Makefile
```

---

## 4. Detailed Steps: Implementation Guide

### Phase 1: Source Code Modification

Perform step-by-step incremental changes, testing regularly.

1. **Create a rollback checkpoint first:**
    ```bash
    git checkout -b build-refactor-traditional
    ```
    
2. **Modify source files safely and incrementally:**

- `src/server.c`:
  - Remove `#include "dtls.c"` and `#include "message.c"` lines.
  - Confirm header guards in `dtls.h` and `message.h` exist or add if missing.

- `src/client.c`:
  - Check and remove any included `.c` file references (if any found).

- Test files (`tests/*.c`):
  - Remove direct inclusion lines: `#include "../src/*.c"`.
  - Verify proper header inclusion instead (`*.h`).

3. **Compile after each incremental step** to catch issues immediately:
```bash
make clean && make
```

4. Repeat above for each component file separately, ensuring every step compiles without warnings before continuing.

---

### Phase 2: Makefile Adaptation and Management

After successful phase 1 steps:

- Clearly identify source/object files in Makefile.
- Define generic compilation rules clearly.
- Explicitly link each executable and test separately.
- Maintain dependency tracking with `-MMD -MP`.

---

## 5. Performance Benchmarking

- Measure and record build times before initiation and post-completion:
```bash
time make clean && make
time make
```

---

## 6. Thorough Verification and Runtime Validation

- Validate build clean state and incremental:
```bash
make clean && make
```
- Ensure binaries run correctly and leak-free (Valgrind).
- Execute full test suite:
```bash
make run-tests
```

---

## 7. Rollback Strategy and Safety

- Document rollback clearly and test regularly:
```bash
git checkout main && git branch -D build-refactor-traditional
```

---

## 8. Documentation Update and Pitfall Record

- Update key documentation explicitly:
  - `docs/README.md`
  - `AGENTS.md`
- Record and troubleshoot typical pitfalls clearly.

---

## 9. Completion Criteria

- ✅ No compile or link errors
- ✅ Successful executable runs
- ✅ Passed Valgrind leak checks
- ✅ Documented incremental performance metrics
- ✅ Updated and accurate documentation
- ✅ Confirmed rollback readiness

---

This refined approach ensures methodical, safe, and transparent refactoring to greatly enhance project maintainability.
