Format all modified C source and header files using GNU indent with the project's coding style.

**Steps to execute:**

1. First, check if GNU indent is installed by running `which indent`. If not found, inform the user that GNU indent needs to be installed.

2. Verify that `.indent.pro` exists in the project root. If not found, inform the user that the indent configuration file is missing.

3. Get the list of modified (staged and unstaged) *.c and *.h files using:
   ```bash
   git diff --name-only --diff-filter=ACMR '*.c' '*.h' && git diff --staged --name-only --diff-filter=ACMR '*.c' '*.h' | sort -u
   ```

4. For each file found, apply GNU indent (it will automatically use .indent.pro):
   ```bash
   indent <filename>
   ```

5. After formatting, show which files were modified by running `git diff --name-only`

**Important notes:**
- GNU indent creates backup files with .BAK extension - remove these after successful formatting
- Only format files that exist and are readable
- If no modified C/H files are found, report "No C or header files to format"
- If any errors occur during formatting, report them but continue with other files
