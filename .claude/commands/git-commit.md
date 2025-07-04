Create a git commit with proper co-authorship attribution and automatically push after approval.

**Arguments:**
- No arguments or `claude`: Use Claude co-author attribution (dynamically determined model)
- `gemini`: Use Gemini co-author attribution
- `codex`: Use Codex co-author attribution
- `claude gemini` (in any order): Use both attributions (output order: Claude, Gemini)
- `claude codex` (in any order): Use both attributions (output order: Claude, Codex)
- `gemini codex` (in any order): Use both attributions (output order: Codex, Gemini)
- `claude gemini codex` (in any order): Use all three attributions (output order: Claude, Codex, Gemini)
- Any other value: Abort with error

**Steps to execute:**

1. Parse arguments and validate:
   ```bash
   # Check if arguments contain claude, gemini, or codex
   HAS_CLAUDE=false
   HAS_GEMINI=false
   HAS_CODEX=false
   INVALID_ARGS=false
   
   for arg in "$@"; do
       if [ "$arg" = "claude" ]; then
           HAS_CLAUDE=true
       elif [ "$arg" = "gemini" ]; then
           HAS_GEMINI=true
       elif [ "$arg" = "codex" ]; then
           HAS_CODEX=true
       else
           INVALID_ARGS=true
       fi
   done
   
   # Check for invalid arguments
   if [ "$INVALID_ARGS" = true ]; then
       echo "ERROR: Invalid arguments. Use: git-commit [claude|gemini|codex] (in any order)"
       exit 1
   fi
   
   # Get CLI versions if needed
   if [ "$HAS_CLAUDE" = true ] || [ $# -eq 0 ]; then
       # Strip "(Claude Code)" from the version output
       CLAUDE_VERSION=$(claude --version | sed 's/ (Claude Code)//')
       # The model ID is dynamically determined based on the current model
       CLAUDE_MODEL="claude-opus-4-20250514"  # This will be dynamically determined
   fi
   if [ "$HAS_GEMINI" = true ]; then
       GEMINI_VERSION=$(gemini --version)
   fi
   if [ "$HAS_CODEX" = true ]; then
       CODEX_VERSION=$(codex --version)
   fi
   
   # Build co-author lines based on flags (consistent output order)
   CO_AUTHORS=""
   
   # No arguments means claude by default
   if [ $# -eq 0 ]; then
       CO_AUTHORS="co-author: claude-cli-${CLAUDE_VERSION} (${CLAUDE_MODEL})"
   else
       # Build co-author lines in alphabetical order: Claude, Codex, Gemini
       if [ "$HAS_CLAUDE" = true ]; then
           CO_AUTHORS="co-author: claude-cli-${CLAUDE_VERSION} (${CLAUDE_MODEL})"
       fi
       
       if [ "$HAS_CODEX" = true ]; then
           if [ -n "$CO_AUTHORS" ]; then
               CO_AUTHORS="${CO_AUTHORS}
co-author: codex-cli-${CODEX_VERSION} (gpt-4.5-preview)"
           else
               CO_AUTHORS="co-author: codex-cli-${CODEX_VERSION} (gpt-4.5-preview)"
           fi
       fi
       
       if [ "$HAS_GEMINI" = true ]; then
           if [ -n "$CO_AUTHORS" ]; then
               CO_AUTHORS="${CO_AUTHORS}
co-author: gemini-cli-${GEMINI_VERSION} (gemini-2.5-pro)"
           else
               CO_AUTHORS="co-author: gemini-cli-${GEMINI_VERSION} (gemini-2.5-pro)"
           fi
       fi
   fi
   ```

2. Check git status to see what files need to be staged:
   ```bash
   git status
   ```

3. Add new and modified files to the staging area:
   ```bash
   git add -A
   ```

4. Show the changes that will be committed:
   ```bash
   git diff --cached
   ```

5. Review recent commits for style consistency:
   ```bash
   git log --oneline -5
   ```

6. Show the proposed commit message to the user:
   ```
   PROPOSED COMMIT MESSAGE:
   ========================
   <title>

   <body>

   <co-author-lines>
   ========================
   ```
   
   Ask the user: "Do you approve this commit message?"

7. Only after user approval, create the commit with co-authorship:
   ```bash
   git commit -m "$(cat <<'EOF'
   <title>

   <body>

   ${CO_AUTHORS}
   EOF
   )"
   ```

8. Verify the commit was created successfully:
   ```bash
   git status
   ```

9. After user approves the commit message, automatically push to remote:
   ```bash
   git push
   ```

**Important notes:**
- ALWAYS show the complete commit message (title, body, and co-author line(s)) to the user BEFORE creating the commit
- Wait for explicit user approval before running `git commit`
- Step 9 (git push) should be executed automatically AFTER the commit is created
- The commit message should use the exact template format: `<title>` followed by blank line, then `<body>`, then co-author line(s)
- For Claude commits, the format is: `claude-cli-<version> (<model>)` where <version> is determined by running `claude --version` and <model> is dynamically determined (e.g., claude-opus-4-20250514)
- For Gemini commits, the format is: `gemini-cli-<version> (gemini-2.5-pro)` where <version> is determined by running `gemini --version`
- For Codex commits, the format is: `codex-cli-<version> (gpt-4.5-preview)` where <version> is determined by running `codex --version`
- Arguments can be provided in any order, but the output co-author lines will always be in alphabetical order: Claude, Codex, Gemini
- The commit message should NOT include type prefixes (like fix:, refactor:, add:, etc.)
- Don't include the 🤖 emoji or Claude Code link as per the updated convention