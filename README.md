# COP4610 – Project 1: UNIX Shell

A small UNIX-like shell implemented in C for Florida State University’s COP4610 (Operating Systems).  
Supports built-ins (`cd`, `jobs`, `exit`), external command execution with PATH search, basic I/O redirection (`<`, `>`), background jobs (`&`), `$VAR` and `~` expansion, and a 3-entry rolling history printed on exit.

---

## Table of Contents
- [Features](#features)
- [Build](#build)
- [Run](#run)
- [Usages](#usages)
---

## Features

### Core
- Prompt `USER@HOST:/cwd>`
- External commands via `fork()` + `execvp()` and PATH search
- Built-ins:
  - `cd [path]` (defaults to `$HOME`, updates `PWD`)
  - `jobs` (lists active background jobs)
  - `exit` (waits for background jobs; prints last 3 commands)
- Environment expansion: tokens beginning with `$VAR`
- Tilde expansion: `~` and `~/...` expand to `$HOME`
- Tokenization is whitespace-based (no quotes/escapes yet)

### I/O & Background
- Input redirection: `< file`
- Output redirection (truncate): `> file`
- Background: `&` (prints `[job_no] pid` and returns prompt)
- Reaping finished background jobs via `check_finished_jobs()` with `WNOHANG`

### Redirection & Pipes
- [x] Input `< file`
- [x] Output `> file` (truncate) and `>> file` (append)
- [x] STDERR `2> file` (optional if required)
- [x] Single and multi-stage pipelines: `cmd1 | cmd2 | cmd3`

### Job Control 
- [x] Background `&` (don’t block prompt)
- [x] `jobs` list active background processes
- [x] `fg %jobno` (bring job to foreground)
- [x] `bg %jobno` (resume stopped job in background)
- [x] Signal handling: ignore `SIGINT` in shell, deliver to foreground group
- [x] Reap children to prevent zombies (`SIGCHLD`)

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

## Build
Tested on Ubuntu/linprog with GCC.
```bash
make clean && make


## Usage

### Basics
```text
user@host:/tmp> pwd
user@host:/tmp> ls -la
user@host:/tmp> cd / && pwd

user@host:/tmp> echo $HOME
user@host:/tmp> ls ~
user@host:/tmp> ls ~/Documents

user@host:/tmp> echo hello > out.txt      # overwrite
user@host:/tmp> cat < out.txt

user@host:/tmp> sleep 2 &
[1] 12345
user@host:/tmp> jobs
# after it finishes:
[1] + done sleep 2

user@host:/tmp> exit
# prints last 3 commands entered during this session




