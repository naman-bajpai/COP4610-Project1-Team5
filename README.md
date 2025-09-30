# COP4610 – Project 1: UNIX Shell

A small UNIX-like shell implemented in C for Florida State University’s COP4610 (Operating Systems).  
Supports built-ins, external command execution, redirection, pipelines, and basic job control.

> Instructor spec varies by section—update the checklist below to reflect what you actually implemented.

---

## Table of Contents
- [Features](#features)
- [Architecture](#architecture)
- [Build](#build)
- [Run](#run)
- [Usage Examples](#usage-examples)
- [Testing](#testing)
- [Project Structure](#project-structure)
- [Design Notes](#design-notes)
- [Troubleshooting](#troubleshooting)
- [Credits](#credits)
- [License](#license)

---

## Features

### Core
- [x] Prompt (e.g., `mysh> `) with clean EOF handling (Ctrl-D)
- [x] Parse and run **external commands** via `fork/execvp`
- [x] Built-ins:
  - [x] `exit` (graceful shutdown)
  - [x] `cd [path]` (defaults to `$HOME`)
  - [x] `pwd`
  - [ ] `echo` (optional)
  - [ ] `history` (optional: show last N)
- [x] PATH search for executables
- [x] Whitespace trimming, basic quoting

### Redirection & Pipes
- [x] Input `< file`
- [x] Output `> file` (truncate) and `>> file` (append)
- [x] STDERR `2> file` (optional if required)
- [x] Single and multi-stage pipelines: `cmd1 | cmd2 | cmd3`

### Job Control (if required by your spec)
- [x] Background `&` (don’t block prompt)
- [x] `jobs` list active background processes
- [x] `fg %jobno` (bring job to foreground)
- [x] `bg %jobno` (resume stopped job in background)
- [x] Signal handling: ignore `SIGINT` in shell, deliver to foreground group
- [x] Reap children to prevent zombies (`SIGCHLD`)

> Update checkboxes to match your implementation.

---

## Architecture

- **Lexer/Parser:** tokenizes input into argv vectors and builds a `Pipeline` object
- **Executor:** sets up redirection & pipes, spawns processes, manages pgid
- **Built-ins:** run in-process (no `fork`) except where noted
- **Jobs:** fixed-size table with `{active, pid, pgid, job_no, cmdline, state}`

Key files (yours may differ):
- `src/finished_shell.c` – REPL, parsing, dispatch, job control
- `src/external_command_execution.c` – exec and I/O plumbing
- `include/*.h` – public structs (e.g., `Pipeline`, `Job`)
- `Makefile` – build targets
- `bin/` and `obj/` – outputs

Diagram (optional): `docs/architecture.drawio.png`

---

## Build

## Run
./bin/shell
# or pass a script file to run batch mode (if you implemented it)
./bin/shell < scripts/demo.sh

### Linux (linprog or Ubuntu)
```bash
make clean && make
# outputs: bin/shell
