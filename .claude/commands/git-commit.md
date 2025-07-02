Create a git commit with proper Claude co-authorship attribution and automatically push after approval.

**Steps to execute:**

1. Check git status to see what files need to be staged:
   ```bash
   git status
   ```

2. Add new and modified files to the staging area:
   ```bash
   git add -A
   ```

3. Show the changes that will be committed:
   ```bash
   git diff --cached
   ```

4. Review recent commits for style consistency:
   ```bash
   git log --oneline -5
   ```

5. Show the proposed commit message to the user:
   ```
   PROPOSED COMMIT MESSAGE:
   ========================
   <title>

   <body>

   co-author: claude-opus-4-20250514
   ========================
   ```
   
   Ask the user: "Do you approve this commit message?"

6. Only after user approval, create the commit with co-authorship:
   ```bash
   # Get the current model ID dynamically
   MODEL_ID="claude-opus-4-20250514"  # This will be dynamically determined by Claude

   git commit -m "$(cat <<'EOF'
   <title>

   <body>

   co-author: ${MODEL_ID}
   EOF
   )"
   ```

7. Verify the commit was created successfully:
   ```bash
   git status
   ```

8. After user approves the commit message, automatically push to remote:
   ```bash
   git push
   ```

**Important notes:**
- ALWAYS show the complete commit message (title, body, and co-author line) to the user BEFORE creating the commit
- Wait for explicit user approval before running `git commit`
- Step 8 (git push) should be executed automatically AFTER the commit is created
- The commit message should use the exact template format: `<title>` followed by blank line, then `<body>`, then co-author line
- The model ID (claude-opus-4-20250514) will be dynamically inserted when creating the commit
- The commit message should NOT include type prefixes (like fix:, refactor:, add:, etc.)
- Don't include the 🤖 emoji or Claude Code link as per the updated convention
