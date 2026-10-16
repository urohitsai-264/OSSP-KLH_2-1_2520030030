# OSSP Shell Skills: ShellForge

A 24-stage C/POSIX shell learning sequence for student ID `2520030030`. Each numbered stage is a separate executable with its own `main`; the final stage is the integrated [ShellForge](ShellForge.c). The pre-existing [Skill-3b.c](Skill-3b.c) tokenizer is preserved as an additional exercise and is also built and tested.

## Build and run

Requires Ubuntu/WSL, GCC, GNU Make, and a POSIX shell.

```sh
make
make test
./build/ShellForge
```

Each program can also be built and run individually, for example:

```sh
gcc -Wall -Wextra -Werror -std=c11 Skill-14.c -o Skill-14
printf 'printf hello | wc -c\n' | ./Skill-14
```

`make clean` removes only the generated `build/` directory. Existing extensionless executables in the project root are left alone.

## Stage map

| Stage | File | Objective |
|---:|---|---|
| 1 | `Skill-1.c` | Read and display a line safely |
| 2 | `Skill-2.c` | Prompt, input loop, and exit |
| 3 | `Skill-3.c` | Bounded in-memory history list |
| 4 | `Skill-4.c` | Whitespace tokenization and argument count |
| 5 | `Skill-5.c` | Empty, oversized, and argument-count validation |
| 6 | `Skill-6.c` | Single/double quotes and backslash escapes |
| 7 | `Skill-7.c` | Create a child with `fork` |
| 8 | `Skill-8.c` | Execute a child and inspect `waitpid` status |
| 9 | `Skill-9.c` | Execute commands through PATH with `execvp` |
| 10 | `Skill-10.c` | Expand `$NAME` from the environment |
| 11 | `Skill-11.c` | `cd` and `pwd` built-ins |
| 12 | `Skill-12.c` | Export an environment assignment with `setenv` |
| 13 | `Skill-13.c` | Dynamic history storage, listing, and `!!` lookup |
| 14 | `Skill-14.c` | One pipe between two commands |
| 15 | `Skill-15.c` | Multi-command pipelines |
| 16 | `Skill-16.c` | Input and truncating output redirection |
| 17 | `Skill-17.c` | Append output and redirect standard error |
| 18 | `Skill-18.c` | Launch a background command |
| 19 | `Skill-19.c` | Track and list background jobs |
| 20 | `Skill-20.c` | Ignore SIGINT in the shell; restore it in children |
| 21 | `Skill-21.c` | Foreground process groups and terminal handoff |
| 22 | `Skill-22.c` | Job table with `jobs`, `fg`, and `bg` operations |
| 23 | `Skill-23.c` | Integrate pipelines, redirection, and background launch |
| 24 | `ShellForge.c` | Integrated shell branded `2520030030_ShellForge` |

## ShellForge

ShellForge supports interactive prompts, `exit`, `help`, `cd`, `pwd`, `export NAME=VALUE`, in-memory `history`, `$NAME` and `$?` expansion, quoted/escaped words, PATH-based external commands, pipelines (up to eight stages), `<`, `>`, `>>`, `2>`, background `&`, job listing, and basic `fg`/`bg` process-group control. Job-control terminal handoff is active when run interactively on a terminal. In scripted input the prompt is suppressed.

This is an educational shell, not a full POSIX shell: it does not implement command substitution, globbing, shell functions, semicolon-separated commands, or full POSIX field splitting. Built-in command redirection is not supported.

## Verification

Run `make test`. The script builds every `Skill-*.c` and `ShellForge.c` separately, then runs a smoke/edge test for every numbered stage and the preserved `Skill-3b.c`. See [test-results.md](test-results.md) for the executed build and test status.
