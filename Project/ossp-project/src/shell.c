#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "command_parser.h"
#include "file_manager.h"
#include "ipc_manager.h"
#include "memory_manager.h"
#include "monitor.h"
#include "process_manager.h"
#include "shell.h"
#include "system_info.h"
#include "thread_manager.h"
#include "trace_manager.h"

static volatile sig_atomic_t shell_exit_flag = 0;

static void shell_signal_handler(int signum) {
    (void)signum;
    shell_exit_flag = 1;
}

void install_shell_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = shell_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int shell_exit_requested(void) {
    return shell_exit_flag;
}

void clear_shell_exit_flag(void) {
    shell_exit_flag = 0;
}

void print_banner(void) {
    printf("============================================================\n");
    printf("        LINUX SYSTEM OPERATIONS CONSOLE\n");
    printf("============================================================\n\n");
    printf("Type 'help' to see available commands.\n\n");
}

static void normalize_command(char *text) {
    if (text == NULL) {
        return;
    }
    for (size_t i = 0; text[i] != '\0'; i++) {
        text[i] = (char)tolower((unsigned char)text[i]);
    }
}

static void print_help(void) {
    printf("SYSTEM\n");
    printf("  help                Show this help screen\n");
    printf("  info                Display real kernel and system information\n");
    printf("  execute <cmd>...    Execute a real Linux command with fork+exec+wait\n");
    printf("  trace <cmd>...      Trace a command using strace when available\n");
    printf("  clear               Clear the terminal\n");
    printf("  exit / quit         Leave the console\n\n");

    printf("PROCESS\n");
    printf("  create              Create a controlled child process\n");
    printf("  status <pid>        Show real process state from /proc\n");
    printf("  tree                Display the process hierarchy\n");
    printf("  zombie             Demonstrate a real zombie process\n");
    printf("  reap                Reap the current zombie child\n");
    printf("  orphan              Demonstrate child reparenting after parent exit\n");
    printf("  signal <type>      Send a real signal to a managed child process\n\n");

    printf("IPC / SIGNALS\n");
    printf("  pipe                Demonstrate anonymous pipe communication\n");
    printf("  fifo                Demonstrate a real named pipe\n\n");

    printf("MEMORY\n");
    printf("  memory              Show real memory statistics from /proc/meminfo\n");
    printf("  vm                  Show the current process virtual address map\n");
    printf("  malloc              Show malloc/calloc/realloc/free behavior\n");
    printf("  cow                 Demonstrate copy-on-write after fork\n");
    printf("  leakcheck          Run Valgrind leak detection\n");
    printf("  leakcheck --fixed  Run the fixed leak-version under Valgrind\n");
    printf("  memcheck           Run a broader Memcheck demonstration\n\n");

    printf("FILESYSTEM\n");
    printf("  file                Demonstrate open/read/write/lseek/close\n");
    printf("  inode               Display inode metadata using stat/fstat\n");
    printf("  mmap                Use mmap()/munmap() on a project file\n\n");

    printf("THREADS / SYNCHRONIZATION\n");
    printf("  thread              Create and join POSIX threads\n");
    printf("  race               Demonstrate a race condition without locking\n");
    printf("  mutex              Demonstrate mutex-protected shared access\n");
    printf("  condition          Demonstrate pthread condition variables\n");
    printf("  semaphore          Demonstrate POSIX semaphores\n");
    printf("  deadlock           Demonstrate a safe deadlock scenario with recovery\n\n");

    printf("MONITORING\n");
    printf("  monitor            Launch the final monitoring dashboard\n");
}

static int dispatch_command(int argc, char **argv) {
    if (argc <= 0 || argv == NULL || argv[0] == NULL) {
        return 0;
    }

    char *cmd = argv[0];
    normalize_command(cmd);

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(cmd, "info") == 0) {
        print_system_info();
        return 0;
    }

    if (strcmp(cmd, "execute") == 0) {
        return cmd_execute(argc, argv);
    }

    if (strcmp(cmd, "trace") == 0) {
        return cmd_trace(argc, argv);
    }

    if (strcmp(cmd, "create") == 0) {
        return cmd_create_process();
    }

    if (strcmp(cmd, "status") == 0) {
        return cmd_status(argc, argv);
    }

    if (strcmp(cmd, "tree") == 0) {
        return cmd_tree();
    }

    if (strcmp(cmd, "zombie") == 0) {
        return cmd_zombie_demo();
    }

    if (strcmp(cmd, "reap") == 0) {
        return cmd_reap_zombie();
    }

    if (strcmp(cmd, "orphan") == 0) {
        return cmd_orphan_demo();
    }

    if (strcmp(cmd, "signal") == 0) {
        return cmd_signal_demo(argc, argv);
    }

    if (strcmp(cmd, "pipe") == 0) {
        return cmd_pipe_demo();
    }

    if (strcmp(cmd, "fifo") == 0) {
        return cmd_fifo_demo();
    }

    if (strcmp(cmd, "memory") == 0) {
        return cmd_memory_info();
    }

    if (strcmp(cmd, "vm") == 0) {
        return cmd_virtual_memory();
    }

    if (strcmp(cmd, "malloc") == 0) {
        return cmd_malloc_demo();
    }

    if (strcmp(cmd, "cow") == 0) {
        return cmd_cow_demo();
    }

    if (strcmp(cmd, "memcheck") == 0) {
        return cmd_memcheck();
    }

    if (strcmp(cmd, "leakcheck") == 0) {
        return cmd_leakcheck(argc, argv);
    }

    if (strcmp(cmd, "file") == 0) {
        return cmd_file_demo();
    }

    if (strcmp(cmd, "inode") == 0) {
        return cmd_inode_demo();
    }

    if (strcmp(cmd, "mmap") == 0) {
        return cmd_mmap_demo();
    }

    if (strcmp(cmd, "thread") == 0) {
        return cmd_thread_demo();
    }

    if (strcmp(cmd, "race") == 0) {
        return cmd_race_demo();
    }

    if (strcmp(cmd, "mutex") == 0) {
        return cmd_mutex_demo();
    }

    if (strcmp(cmd, "condition") == 0) {
        return cmd_condition_demo();
    }

    if (strcmp(cmd, "semaphore") == 0) {
        return cmd_semaphore_demo();
    }

    if (strcmp(cmd, "deadlock") == 0) {
        return cmd_deadlock_demo();
    }

    if (strcmp(cmd, "monitor") == 0) {
        return cmd_monitor();
    }

    if (strcmp(cmd, "clear") == 0) {
        printf("\033[H\033[J");
        fflush(stdout);
        return 0;
    }

    if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
        return 1;
    }

    printf("ERROR: Unknown command '%s'. Type 'help' for supported commands.\n", cmd);
    return 0;
}

int shell_run(void) {
    char input[512];
    char *argv[CMD_MAX_ARGS];
    int argc = 0;

    while (!shell_exit_requested()) {
        printf("linux-os> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nGoodbye.\n");
            break;
        }

        if (parse_command(input, argv, &argc) != 0) {
            printf("ERROR: Failed to parse command.\n");
            continue;
        }

        if (argc == 0) {
            continue;
        }

        if (dispatch_command(argc, argv) != 0) {
            for (int i = 0; i < argc; i++) {
                free(argv[i]);
            }
            break;
        }

        for (int i = 0; i < argc; i++) {
            free(argv[i]);
        }
    }

    return 0;
}
