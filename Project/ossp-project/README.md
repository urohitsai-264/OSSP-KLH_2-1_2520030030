# Linux System Operations Console

## Project title

LINUX SYSTEM OPERATIONS CONSOLE

## Problem statement

The original project was a dashboard-style Linux monitor that started automatically and did not behave like a real command-driven operating systems console. The redesigned project transforms it into a genuine Linux command shell that demonstrates real process lifecycles, IPC, memory management, file I/O, threads, synchronization, tracing, and final system monitoring using actual Linux/POSIX operations.

## Motivation

Operating systems concepts are best understood by observing real behavior on Linux rather than by simulating values. The new design emphasizes authentic system interaction with `fork()`, `exec()`, `waitpid()`, `pipe()`, `mkfifo()`, `kill()`, `pthread_create()`, `mmap()`, `malloc()`, `stat()`, and `strace`. This makes the project suitable for viva demonstration and a serious external presentation.

## Objectives

- Build a professional command-driven Linux console with a prompt: `linux-os>`
- Demonstrate real OS behavior instead of static dashboard output
- Cover CO1 through CO6 using Linux/POSIX APIs and `/proc`
- Show process lifecycle states including zombie and orphan cases
- Demonstrate IPC, memory, threads, and file-system behavior with real system calls
- Provide a final monitor only when explicitly requested
- Ensure correct cleanup of processes, FIFOs, temporary files, and memory

## Features

- Shell with `help`, `info`, `execute`, `trace`, `create`, `status`, `tree`, `zombie`, `reap`, `orphan`, `signal`
- Anonymous pipe and FIFO demonstrations
- Real memory statistics from `/proc/meminfo`
- Virtual address space inspection via `/proc/self/maps`
- Dynamic allocation demo using `malloc()`, `calloc()`, `realloc()`, and `free()`
- `Valgrind` leak detection with `leakcheck` and `leakcheck --fixed`
- Memory error checking via `memcheck`
- File I/O and inode inspection using `open()`, `read()`, `write()`, `lseek()`, `stat()`, `fstat()`, and `mmap()`
- POSIX threads, race condition, mutex, condition variables, semaphores, and safe deadlock demonstration
- Final monitoring dashboard launched by command, not by default

## Architecture

The project is split into modular C files:

- `src/main.c`: application entry point
- `src/shell.c`: interactive command loop and command dispatch
- `src/command_parser.c`: input parsing and argument splitting
- `src/system_info.c`: uname/hostname/uptime system output
- `src/process_manager.c`: process creation, status, tree, zombie/reap, orphan, and signal commands
- `src/ipc_manager.c`: pipe and FIFO demonstrations
- `src/memory_manager.c`: RAM, VM, allocation, COW, leakcheck, memcheck
- `src/file_manager.c`: file operations, inode metadata, mmap demo
- `src/thread_manager.c`: thread creation, race, mutex, condvar, semaphore, deadlock
- `src/trace_manager.c`: real `strace` invocation and filtered summary
- `src/monitor.c`: final consolidated monitoring dashboard

## CO mapping

| CO | Concept | Covered by |
| --- | --- | --- |
| CO1 | Command execution, system interaction | `execute`, `info`, `trace` |
| CO2 | Process lifecycle, states, hierarchy, zombie, orphan, signals | `create`, `status`, `tree`, `zombie`, `reap`, `orphan`, `signal` |
| CO3 | IPC | `pipe`, `fifo` |
| CO4 | Memory and Valgrind | `memory`, `vm`, `malloc`, `cow`, `memcheck`, `leakcheck` |
| CO5 | File system and file I/O | `file`, `inode`, `mmap` |
| CO6 | Threads and synchronization | `thread`, `race`, `mutex`, `condition`, `semaphore`, `deadlock` |

## Commands

```text
help
info
execute <command> [args...]
trace <command> [args...]
create
status <pid>
tree
zombie
reap
orphan
signal <SIGINT|SIGTERM|SIGUSR1|SIGUSR2|SIGCHLD>
pipe
fifo
memory
vm
malloc
cow
memcheck
leakcheck
leakcheck --fixed
file
inode
mmap
thread
race
mutex
condition
semaphore
deadlock
monitor
clear
exit
```

## Technologies

- GCC / Make
- C11
- POSIX threads (`pthread_*`)
- Linux `/proc` filesystem
- POSIX file APIs
- `strace` and `Valgrind` for system-call and memory tracing

## Linux / POSIX APIs used

- `fork()`
- `execvp()`
- `waitpid()`
- `kill()`
- `pipe()`
- `mkfifo()`
- `open()` / `read()` / `write()` / `lseek()` / `close()`
- `mmap()` / `munmap()`
- `malloc()` / `calloc()` / `realloc()` / `free()`
- `pthread_create()` / `pthread_join()`
- `pthread_mutex_lock()` / `pthread_mutex_unlock()`
- `pthread_cond_wait()` / `pthread_cond_signal()`
- `sem_init()` / `sem_wait()` / `sem_post()`
- `stat()` / `fstat()`
- `uname()` / `gethostname()` / `sysconf()`

## Build instructions

```bash
make clean
make
```

## Run instructions

```bash
./ossp_monitor
```

The application starts with the banner and a `linux-os>` prompt. It remains command-driven until the user enters `exit` or `quit`.

## Valgrind installation and usage

Install Valgrind on Ubuntu/WSL:

```bash
sudo apt update
sudo apt install valgrind
```

Run the built-in checks:

```bash
./ossp_monitor
linux-os> leakcheck
linux-os> leakcheck --fixed
linux-os> memcheck
```

The console compiles isolated demo programs under `demos/` and executes them with real Valgrind Memcheck output.

## strace usage

Install `strace` if needed:

```bash
sudo apt install strace
```

Example:

```bash
./ossp_monitor
linux-os> trace ls
linux-os> trace pwd
linux-os> trace cat /etc/os-release
```

The tool runs the real `strace` utility and displays a filtered summary of actual system calls such as `execve`, `openat`, `read`, and `close`.

## Testing

A smoke test is provided in `tests/smoke_test.sh`.

```bash
chmod +x tests/smoke_test.sh
./tests/smoke_test.sh
```

Example runtime checks:

```bash
printf 'help\ninfo\nexit\n' | ./ossp_monitor
printf 'create\nstatus $(...)\nexit\n' | ./ossp_monitor
```

## Safety considerations

- No root privileges required
- No arbitrary system process termination
- No dangerous system file modification
- Temporary files are created in the project directory or `/tmp`
- Zombie candidates are reaped immediately after demonstration
- Deadlock demonstration is isolated and timeout-safe
- Main console remains stable during all demos

## Limitations

- The project is designed for Linux and WSL; some system-specific details vary by distribution
- Some process state information depends on the current environment and permissions
- `strace` and `Valgrind` must be installed for those demonstrations to run fully
- `monitor` is a real observation dashboard and not a shortcut around the command shell

## Future enhancements

- Add richer `ps`-like process filtering and tree formatting
- Add a filesystem directory traversal command
- Add real `SIGALRM`-based timed process monitoring
- Improve pretty output and color support
- Add a dedicated documentation folder for detailed viva explanations

## Suggested viva sequence

```bash
./ossp_monitor
linux-os> help
linux-os> info
linux-os> execute ls
linux-os> create
linux-os> status <pid>
linux-os> tree
linux-os> zombie
linux-os> status <pid>
linux-os> reap
linux-os> orphan
linux-os> pipe
linux-os> fifo
linux-os> memory
linux-os> vm
linux-os> malloc
linux-os> cow
linux-os> leakcheck
linux-os> leakcheck --fixed
linux-os> trace ls
linux-os> file
linux-os> inode
linux-os> mmap
linux-os> thread
linux-os> race
linux-os> mutex
linux-os> condition
linux-os> semaphore
linux-os> deadlock
linux-os> monitor
exit
```
