Create a git commit with proper co-authorship attribution and automatically push after approval.

**Arguments:**
- No arguments or `claude`: Use Claude co-author attribution (dynamically determined model)
- `gemini`: Use Gemini co-author attribution
- `claude gemini` or `gemini claude`: Use both attributions (Claude first, then Gemini)
- Any other value: Abort with error

**Steps to execute:**

1. Parse arguments and validate:
   ```bash
   # Check if arguments contain both claude and gemini
   HAS_CLAUDE=false
   HAS_GEMINI=false
   
   for arg in "$@"; do
       if [ "$arg" = "claude" ]; then
           HAS_CLAUDE=true
       elif [ "$arg" = "gemini" ]; then
           HAS_GEMINI=true
       fi
   done
   
   # Get gemini CLI version if needed
   if [ "$HAS_GEMINI" = true ]; then
       GEMINI_VERSION=$(gemini --version)
   fi
   
   # Check arguments and set co-author lines
   if [ $# -eq 0 ] || ([ $# -eq 1 ] && [ "$1" = "claude" ]); then
       # Claude only (default)
       CO_AUTHORS="co-author: claude-opus-4-20250514"  # Model ID will be dynamically determined
   elif [ $# -eq 1 ] && [ "$1" = "gemini" ]; then
       # Gemini only
       CO_AUTHORS="co-author: gemini-cli-${GEMINI_VERSION} (gemini-2.5-pro)"
   elif [ $# -eq 2 ] && [ "$HAS_CLAUDE" = true ] && [ "$HAS_GEMINI" = true ]; then
       # Both Claude and Gemini (always Claude first regardless of argument order)
       CO_AUTHORS="co-author: claude-opus-4-20250514
co-author: gemini-cli-${GEMINI_VERSION} (gemini-2.5-pro)"
   else
       echo "ERROR: Invalid arguments. Use: git-commit [claude|gemini|'claude gemini'|'gemini claude']"
       exit 1
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
- For Claude commits, the model ID will be dynamically determined (e.g., claude-opus-4-20250514)
- For Gemini commits, the format is: `gemini-cli-<version> (gemini-2.5-pro)` where <version> is determined by running `gemini --version`
- When both are specified, Claude co-author line comes first
- The commit message should NOT include type prefixes (like fix:, refactor:, add:, etc.)
- Don't include the 🤖 emoji or Claude Code link as per the updated convention