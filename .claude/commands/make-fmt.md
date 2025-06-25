Format all C source and header files in the project using the makefile's fmt target.

**Steps to execute:**

1. Run the makefile's fmt target:
   ```bash
   make fmt
   ```

2. Show which files were modified by the formatting:
   ```bash
   git diff --name-only
   ```

**Important notes:**
- This will format all .c and .h files in the project (excluding the vendor/ directory)
- GNU indent may create backup files with .BAK extension - these should be cleaned up after formatting
- The makefile's fmt target uses GNU indent with the project's .indent.pro configuration