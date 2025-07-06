![](https://github.com/mgreenly/space-captain/blob/faa6752349538bfa05d595e004a8d658b0f5098d/dat/spacecaptain-01.png)

Space Captain
=============

Space Captain is a toy MMO project built in C as a technical exploration of Linux network programming, systems optimization and game design. The project will evolve from a foundational technical demo (v0.1.0) featuring CLI-based space combat with 1000's concurrent client connections, through platform-specific graphical clients (v0.2.0-0.4.0), to a persistent world with crew progression, economy, and manufacturing systems (v0.5.0+).

You can follow along at https://www.youtube.com/@SpaceCaptainDevLog

Installing
==========

```
git clone https://github.com/mgreenly/space-captain.git
cd space-captian
make clean; PREFIX=$HOME/.local/bin make install
```

Dependencies
============

If you want everything to work the way it does for me you'll need these things installed.

## Prerequisites
- GNU coreutils package (assumed to be installed)
  - Provides: mkdir, rm, ln, install, echo, cat, cut, date, readlink, basename, test/[ ]

## Core Build Tools
- gcc - GNU C Compiler (used as CC)
- make - GNU Make build automation tool

## Development Tools
- gdb - GNU Debugger (for debug-server and debug-client targets)
- clang-format - Code formatter (for fmt target)
- dot - Graphviz diagram generator (for generating arch.png)
- direnv - Environment variable management (.envrc file loads node_modules/.bin to PATH)

## Package Management
- npm - Node Package Manager (for install-tools and update-tools)

## Shell Features
- find - File search utility (may be part of findutils)
- Shell built-ins: while, read, for loops

## Environment Variables Used
- HOME - User's home directory (via shell echo $$HOME)

## NPM Packages Referenced (not direct tools but installed via npm)
- @google/gemini-cli
- @anthropic-ai/claude-code
- @openai/codex
