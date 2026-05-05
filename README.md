# Snail Shell

Snail is a beginner-friendly Unix-like shell written primarily in C. It is designed to help users work with shell commands in a safer and more guided way, while still teaching the core ideas behind how shells function internally. The project already supports a working REPL, built-in commands, external command execution, redirection, pipelines, history, prompt customization, and a safe dump-based delete system.

The main idea behind Snail is simple: a shell should not only execute commands, but also make users feel more in control. That is why this project focuses on clearer command behavior, user-facing configuration, and safer file operations. Its two most novel features today are:

- `file dump` instead of immediate permanent deletion
- `interactive delete` for guided cleanup inside a directory

Snail is currently aimed at Unix-like systems because it relies on POSIX features such as `fork`, `execvp`, `dup2`, `termios`, `waitpid`, signals, and directory APIs. It is not a native Windows shell, but it should work on Windows through WSL.

## Main Language and Tech Stack

- Primary language: `C`
- Build system: `CMake`
- Supporting language: `Python`
- Key system concepts used: `fork`, `execvp`, `waitpid`, `dup2`, `signal`, `termios`, `opendir`, `readdir`, `stat`

This is mostly a systems programming project in C. Python is used mainly for automated testing and presentation/documentation support.

## Project Goals

Snail is trying to be:

- beginner friendly
- safer than a minimal academic shell
- modular enough to extend feature by feature
- visually configurable
- a stepping stone toward a more interactive learning shell

In other words, this project is not only about reproducing a normal shell. It is about building a shell that helps newer users avoid mistakes and understand what is happening.

## Current Functioning

At the moment, Snail works as an interactive shell with the following flow:

1. `main.c` starts the shell and installs `SIGINT` handling.
2. `repl.c` enters the read-evaluate-print loop.
3. Input is read in raw terminal mode so arrow keys, tab completion, and prompt redraws can be handled manually.
4. The parser converts text into a `Command` or `Pipeline`.
5. The executor decides whether the command is a built-in or an external command.
6. Built-ins run inside the shell process when needed, while external commands are launched with `fork` + `execvp`.
7. Prompt metadata such as time, last status, duration, and git branch are refreshed between commands.
8. Command history is loaded from and saved to the user's home directory.

### What already works

- interactive REPL
- command history with persistence
- up/down arrow history navigation
- left/right cursor movement
- `Tab`-based path completion
- `Ctrl+L` clear screen
- `Ctrl+D` exit shell
- support for built-in commands
- support for external Unix commands
- pipelines using `|`
- input redirection using `<`
- output redirection using `>`
- append redirection using `>>`
- user configuration through `~/snailShellrc`
- startup commands loaded from config
- safe delete through `~/.snailDump`
- interactive delete mode

## Why Snail Is Beginner Friendly

Snail tries to lower the barrier for users who are not very comfortable with traditional shells yet.

- It keeps important file deletion safer by moving deleted items into a dump instead of permanently erasing them immediately.
- It uses explicit `mv` modes like `-re`, `-shift`, and `-over` instead of expecting users to remember overloaded standard shell behavior.
- It gives visible prompt feedback such as current path, git branch, time, last exit code, and duration.
- It lets users change appearance and prompt behavior without touching the source code.
- It includes a help command and a readable config template that is copied on first run.

This makes Snail feel more guided than a minimal shell while still exposing real shell concepts.

## Novelty Features

### 1. File Dump

Instead of permanently deleting files right away, Snail routes deletions into a dump folder:

- deleted content goes to `~/.snailDump/files`
- metadata goes to `~/.snailDump/info`
- each deleted item gets a timestamped record
- cleanup can happen automatically on startup
- current retention logic removes dump contents older than 48 hours when cleanup is enabled

This makes `rm` and `rmdir` safer than direct destructive deletion. Internally, the dump system is implemented mainly in `src/fileDump.c`.

### 2. Interactive Delete

Snail includes an interactive deletion mode through `rm -i <directory>`.

Current behavior:

- scans files in the given directory
- ranks candidates using a score based on age and size
- shows up to 10 entries
- lets the user move with arrow keys
- lets the user select entries with `Space`
- confirms deletion with `Enter`
- asks for a final `Y/N` confirmation
- still sends chosen files to the dump instead of deleting them permanently

This is one of the most distinctive user-facing features in the project because it turns deletion into a guided cleanup workflow instead of a blind one-shot command.

## UI and Configuration

Snail is configurable by the user through `~/snailShellrc`. On first run, the shell creates this file automatically using `snail.conf` as the template if needed.

The config currently supports:

- theme colors
- prompt color
- directory color
- separate `ls` colors for files and directories
- separator text
- path display style
- whether to show user name
- whether to show hostname
- one-line vs two-line prompt
- git branch display
- exit status display
- last command duration display
- clock/time display
- startup commands
- font family
- font size
- preferred window width
- preferred window height
- whether startup ASCII snail art is shown
- shell startup cleanup behavior

### Prompt behavior

The prompt is one of the key design areas of the project. It can show:

- current path
- git branch
- last non-zero exit status
- last command runtime
- current time

Supported path styles:

- `full`
- `home_relative`
- `basename`
- `shortened`

Snail also supports a rainbow path mode for directory segments, which makes the prompt more visually informative and customizable.

### Reloading config

You do not need to restart the shell after every config change. Snail includes:

- `reloadConfig`

This reloads `~/snailShellrc` during the active session.

## Basic Commands

Snail is not trying to replace the entire Unix shell ecosystem yet, but it already supports a useful beginner-level command set.

### Built-in commands

- `cd <dir>`: change current directory
- `ls [dir]`: list files
- `ls -l [dir]`: long-style listing with ownership, size, and time
- `mkdir <dir>`: create directory
- `touch <file>`: create file or update timestamp
- `mv -re <src> <newname>`: rename file
- `mv -shift <src> <dir>`: move file into another directory
- `mv -over <src> <dest>`: overwrite one file with another, with dump backup safety
- `rm <path>`: move file to dump
- `rm -r <dir>`: move directory to dump
- `rm -f <path>`: force missing-path tolerance
- `rm -rf <dir>`: recursive directory removal through dump
- `rm -i <directory>`: interactive deletion selection mode
- `rmdir <dir>`: remove empty directory through dump
- `tree [dir]`: show directory tree
- `clear`: clear terminal
- `snailHelp`: show help
- `reloadConfig`: reload config file
- `exit`: exit shell

### Shell syntax already supported

- `cmd1 | cmd2`
- `cmd > file`
- `cmd >> file`
- `cmd < file`

### External commands

If a command is not built into Snail, the shell tries to run it as an external Unix command using `execvp`. That means commands like `echo`, `pwd`, `cat`, `tr`, and `wc` can work if they are available in the system environment.

## Internal Architecture

The codebase is intentionally modular so different parts of shell behavior can be developed independently.

### Core execution path

- `src/main.c`
  - shell entry point
  - installs `SIGINT` handler
- `src/repl.c`
  - main shell loop
  - raw input mode
  - history navigation
  - cursor movement
  - tab completion
  - startup command execution
- `src/parser.c`
  - tokenizes commands
  - parses pipes
  - parses redirection fields
- `src/executor.c`
  - dispatches built-ins
  - handles built-in redirection
  - manages pipelines
- `src/externalCommand.c`
  - forks and runs external programs

### Prompt and UX

- `src/prompt.c`
  - themed prompt rendering
  - git branch discovery
  - time display
  - last status display
  - last duration display
- `src/commandHistory.c`
  - persistent history in `~/.snailCHistory`

### File operation and safety layer

- `src/rmCommand.c`
  - `rm` flags
  - interactive deletion UI
- `src/rmdirCommand.c`
  - empty-directory checks
  - dump-based removal
- `src/fileDump.c`
  - dump creation
  - dump cleanup
  - metadata storage
  - internal restore helper logic
- `src/mvCommand.c`
  - guided move/rename/overwrite modes
  - destination backup through dump before overwrite

### Configuration and help

- `src/config.c`
  - default config
  - config bootstrap
  - config loading from `~/snailShellrc`
- `src/reloadConfig.c`
  - live config reload
- `src/snailHelp.c`
  - in-shell help text

## File Structure

The structure below focuses on the main working shell code and intentionally ignores the auxiliary `ui_snail/` folder in the breakdown.

```text
Snail/
|-- CMakeLists.txt                          # CMake build definition
|-- snail.conf                             # default config template copied to ~/snailShellrc
|-- include/                               # header files
|   |-- parser.h                           # Command and Pipeline structures
|   |-- executor.h                         # execution interface
|   |-- config.h                           # config structure and loader API
|   |-- prompt.h                           # prompt metadata interface
|   |-- commandHistory.h                   # persistent history API
|   |-- fileDump.h                         # dump and restore helpers
|   |-- rmCommand.h / rmdirCommand.h       # delete features
|   |-- mvCommand.h                        # guided move features
|   |-- lsCommand.h / treeCommand.h        # file listing and tree output
|   |-- cdCommand.h / mkdirCommand.h       # directory operations
|   |-- touchCommand.h / clearCommand.h    # utility built-ins
|   |-- reloadConfig.h / snailHelp.h       # shell usability commands
|   `-- terminal.h                         # raw mode helper declarations
|-- src/                                   # main C implementation
|   |-- main.c                             # shell entry point
|   |-- repl.c                             # input loop and interactive editing
|   |-- parser.c                           # parser for commands, pipes, redirects
|   |-- executor.c                         # built-in dispatcher and pipeline runner
|   |-- externalCommand.c                  # external process execution
|   |-- prompt.c                           # customizable prompt rendering
|   |-- config.c                           # config bootstrap and parsing
|   |-- commandHistory.c                   # history persistence
|   |-- rmCommand.c                        # safe rm + interactive delete
|   |-- rmdirCommand.c                     # empty directory removal via dump
|   |-- fileDump.c                         # dump storage and cleanup logic
|   |-- mvCommand.c                        # rename / move / overwrite flows
|   |-- lsCommand.c                        # listing implementation
|   |-- treeCommand.c                      # tree command
|   |-- cdCommand.c                        # cd behavior
|   |-- mkdirCommand.c                     # mkdir behavior
|   |-- touchCommand.c                     # touch behavior
|   |-- clearCommand.c                     # clear behavior
|   |-- reloadConfig.c                     # reload config command
|   |-- snailHelp.c                        # built-in help output
|   `-- exitCommand.c                      # shell exit
|-- tests/
|   `-- run_session_regression_tests.py    # Python regression and runtime test suite
|-- presentation/
|   |-- generate_group16_presentation.py   # auto-generates project presentation PDF
|   |-- EDITABLE_PRESENTATION_NOTES.md     # notes for editing presentation content
|   `-- Snail_Group16_Presentation.pdf     # generated presentation output
|-- session_change_log.json                # progress/change tracking artifact
|-- history.txt                            # sample or local history artifact
`-- snail / a.out                          # local build outputs
```

## Build and Run

### Using CMake

```bash
cmake -S . -B build
cmake --build build
./build/snail
```

### Using GCC directly

```bash
gcc -Wall -Wextra -Iinclude src/*.c -o snail
./snail
```

### On Windows

Use WSL and build from the Linux side, for example:

```bash
cd /mnt/c/Users/ASUS/OneDrive/Documents/GitHub/Snail
gcc -Wall -Wextra -Iinclude src/*.c -o snail
./snail
```

## Design Decisions

Several design decisions make Snail different from a very small classroom shell.

### 1. Safe delete over destructive delete

The shell chooses safety by default. Instead of using permanent deletion immediately, it routes content to a structured dump. This is especially useful for beginner users who may mistype paths.

### 2. Explicit move modes

The custom `mv` interface uses:

- `-re`
- `-shift`
- `-over`

This is a deliberate beginner-friendly choice. It separates rename, move, and overwrite into explicit user intentions instead of hiding them behind a more overloaded default behavior.

### 3. Modular architecture

Parsing, execution, prompt rendering, config loading, history, and dump handling are all split into separate files. This makes the project easier to test, debug, and extend.

### 4. Config-first UI behavior

Rather than hardcoding the shell look, Snail copies a config template to the user's home directory and reads appearance settings at runtime. That means users can change the UI feel without recompilation.

### 5. Interactive shell as a learning tool

The shell is built not just to execute commands, but to help users understand shell interactions gradually through visible prompt metadata, safer deletion, and guided command flows.

## Development Timeline of the Project

From the current code structure, test suite, and presentation material, the project appears to have evolved in clear stages:

### Phase 1. Core shell foundation

- shell entry point
- REPL loop
- raw input support
- parser and executor split

This phase established the minimum shell workflow.

### Phase 2. Basic built-ins and external execution

- `cd`
- `ls`
- `mkdir`
- `touch`
- `clear`
- `exit`
- external command execution with `fork` and `execvp`

This made Snail usable as a day-to-day shell prototype.

### Phase 3. Shell syntax features

- pipelines
- input redirection
- output redirection
- append redirection

This moved the project closer to standard shell expectations.

### Phase 4. User experience and personalization

- persistent history
- prompt colors
- time, status, duration, and git branch in prompt
- startup commands
- reloadable user config

This phase made the shell more interactive and user-specific.

### Phase 5. Safety-oriented novelty features

- file dump
- recursive safe removal
- dump metadata
- auto cleanup
- interactive delete mode
- safer overwrite flow in `mv`

This is the phase where Snail became more distinct from a normal student shell.

### Phase 6. Testing and presentation maturity

- automated regression test script in Python
- generated presentation assets
- documentation and change tracking artifacts

This phase reflects the project becoming more polished and review-ready.

## Automated Test Suite

Snail includes a substantial Python-based regression script:

- `tests/run_session_regression_tests.py`

The script is best suited to Unix-like environments or WSL because it uses tools and behaviors such as `zsh`, pseudo-terminals, and temporary Unix-style paths.

### What this script does

- builds a temporary shell binary with `gcc`
- runs interactive shell sessions inside a pseudo-terminal
- strips ANSI escape codes for stable assertions
- creates isolated temporary `HOME` and working directories
- verifies both runtime behavior and some source-level implementation contracts

### What it covers

The script defines `32` checks (`TC-001` to `TC-032`) covering areas such as:

- clean GCC build
- CMake configure/build
- startup and exit
- config bootstrap
- startup commands
- help command output redirection
- config reload
- input/output/append redirection
- pipelines
- built-ins inside pipelines
- `cd` home expansion
- `ls` behavior
- `rm` dump behavior
- directory removal behavior
- `mv` modes
- history persistence
- parser redirection support
- executor redirection helper wiring
- prompt config feature presence
- CMake target setup

This is a strong sign that the project is not only implemented, but also being validated in a structured way.

## Current Limitations

Snail is functional, but it is still evolving. A few practical limitations are worth noting:

- it targets Unix-like environments, not native Windows APIs
- it depends on POSIX terminal/process behavior
- it is not a full Bash replacement
- parser behavior is still simple compared to mature shells
- dump restore logic exists internally, but there is not yet a full user-facing restore command wired into the shell
- `dumpList` exists in source form but is currently not active in the command dispatcher

These are normal tradeoffs for a shell project at this stage.

## Future Scope

The project already points naturally toward several next steps.

### Learning and safety

- add a true sandbox mode so users can experiment without harming real files
- support dry-run style previews for risky commands
- expand guided help and onboarding flows

### System visibility and control

- add a built-in command to view current running processes in real time
- allow users to inspect and control those processes from inside Snail
- grow toward a lightweight `ps`/`top`-style internal monitor

### File and dump improvements

- expose restore as a user-facing command
- add dump listing and search
- support smarter retention policies
- consider dump compression for larger workloads

### UX and shell power

- stronger autocomplete
- aliases
- better quoting/parsing support
- richer interactive UI settings
- more internal help and discoverability

## Why This Project Matters

Snail is a good example of a systems project that does more than simply satisfy shell basics. It shows how a beginner-oriented product idea can be combined with low-level OS concepts. The project teaches how shells work internally while also asking an important product question: how can a shell be made safer and more welcoming for less experienced users?

That combination of systems programming, user empathy, and extensibility is what gives Snail its identity.
