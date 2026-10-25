#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "process_manager.h"

static pid_t g_created_pid = -1;
static pid_t g_zombie_pid = -1;

static int parse_proc_status_num(const char *path, const char *key, long *value) {
    FILE *fp = fopen(path, "r");
    char line[256];

    if (fp == NULL) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, key, strlen(key)) == 0) {
            char *pos = strchr(line, ':');
            if (pos != NULL) {
                long parsed = 0;
                char *end = NULL;
                pos++;
                while (*pos == ' ' || *pos == '\t') {
                    pos++;
                }
                errno = 0;
                parsed = strtol(pos, &end, 10);
                fclose(fp);
                if (errno == 0 && end != pos) {
                    *value = parsed;
                    return 0;
                }
                return -1;
            }
        }
    }

    fclose(fp);
    return -1;
}

static int parse_proc_status_str(const char *path, const char *key, char *out, size_t out_size) {
    FILE *fp = fopen(path, "r");
    char line[256];

    if (fp == NULL) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, key, strlen(key)) == 0) {
            char *pos = strchr(line, ':');
            if (pos != NULL) {
                pos++;
                while (*pos == ' ' || *pos == '\t') {
                    pos++;
                }
                snprintf(out, out_size, "%s", pos);
                size_t len = strlen(out);
                while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r')) {
                    out[--len] = '\0';
                }
                fclose(fp);
                return 0;
            }
        }
    }

    fclose(fp);
    return -1;
}

int cmd_create_process(void) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        printf("[child] PID=%ld: spawned and sleeping for 30 seconds.\n", (long)getpid());
        fflush(stdout);
        sleep(30);
        _exit(0);
    }

    g_created_pid = pid;
    printf("Parent PID     : %ld\n", (long)getpid());
    printf("Child PID      : %ld\n", (long)pid);
    printf("Initial state  : running\n");
    printf("Managed child : active\n");
    return 0;
}

int cmd_status(int argc, char **argv) {
    char status_path[PATH_MAX];
    char name[128] = {0};
    char state[32] = {0};
    long ppid = -1;
    long threads = -1;
    pid_t pid;

    if (argc != 2) {
        fprintf(stderr, "Usage: status <pid>\n");
        return -1;
    }

    pid = (pid_t)strtol(argv[1], NULL, 10);
    if (pid <= 0) {
        fprintf(stderr, "ERROR: invalid PID '%s'.\n", argv[1]);
        return -1;
    }

    snprintf(status_path, sizeof(status_path), "/proc/%ld/status", (long)pid);
    if (access(status_path, F_OK) != 0) {
        fprintf(stderr, "ERROR: PID %ld is not available in /proc.\n", (long)pid);
        return -1;
    }

    if (parse_proc_status_str(status_path, "Name", name, sizeof(name)) != 0) {
        snprintf(name, sizeof(name), "unknown");
    }
    if (parse_proc_status_str(status_path, "State", state, sizeof(state)) != 0) {
        snprintf(state, sizeof(state), "unknown");
    }
    if (parse_proc_status_num(status_path, "PPid", &ppid) != 0) {
        ppid = -1;
    }
    if (parse_proc_status_num(status_path, "Threads", &threads) != 0) {
        threads = -1;
    }

    printf("PID           : %ld\n", (long)pid);
    printf("PPID          : %ld\n", ppid);
    printf("State         : %s\n", state);
    printf("Name          : %s\n", name);
    printf("Threads       : %ld\n", threads);
    return 0;
}

int cmd_tree(void) {
    DIR *dir = opendir("/proc");
    struct dirent *entry = NULL;
    char name[128];
    int count = 0;

    if (dir == NULL) {
        perror("opendir /proc");
        return -1;
    }

    printf("Process tree (real /proc hierarchy):\n");
    while ((entry = readdir(dir)) != NULL) {
        char status_path[PATH_MAX];
        long ppid = -1;
        if (entry->d_name[0] == '\0' || !isdigit((unsigned char)entry->d_name[0])) {
            continue;
        }

        snprintf(status_path, sizeof(status_path), "/proc/%s/status", entry->d_name);
        if (access(status_path, R_OK) != 0) {
            continue;
        }

        if (parse_proc_status_str(status_path, "Name", name, sizeof(name)) != 0) {
            continue;
        }

        if (parse_proc_status_num(status_path, "PPid", &ppid) == 0) {
            if (ppid == 0 || ppid == 1 || ppid == getpid()) {
                printf("%s\n", name);
                count++;
            }
        }
    }

    closedir(dir);
    printf("Processes shown: %d\n", count);
    return 0;
}

int cmd_zombie_demo(void) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        _exit(0);
    }

    g_zombie_pid = pid;
    sleep(1);

    printf("ZOMBIE PROCESS DEMONSTRATION\n");
    printf("Parent PID : %ld\n", (long)getpid());
    printf("Child PID  : %ld\n", (long)pid);
    printf("Child has terminated.\n");

    char status_path[PATH_MAX];
    snprintf(status_path, sizeof(status_path), "/proc/%ld/status", (long)pid);
    if (access(status_path, F_OK) == 0) {
        char state[32] = {0};
        if (parse_proc_status_str(status_path, "State", state, sizeof(state)) == 0) {
            printf("Current state: %s\n", state);
        }
    }

    printf("Explanation: the child has exited, but the parent has not yet called waitpid().\n");
    return 0;
}

int cmd_reap_zombie(void) {
    int status = 0;

    if (g_zombie_pid <= 0) {
        fprintf(stderr, "ERROR: No zombie process is active to reap.\n");
        return -1;
    }

    if (waitpid(g_zombie_pid, &status, 0) == -1) {
        perror("waitpid");
        return -1;
    }

    printf("Child PID %ld reaped successfully.\n", (long)g_zombie_pid);
    printf("Zombie state cleared.\n");
    g_zombie_pid = -1;
    return 0;
}

int cmd_orphan_demo(void) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        char path[PATH_MAX];
        long parent_pid_after = -1;

        snprintf(path, sizeof(path), "/proc/%ld/status", (long)getppid());
        if (access(path, F_OK) == 0) {
            char parent_name[128] = {0};
            parse_proc_status_str(path, "Name", parent_name, sizeof(parent_name));
            printf("[child] Parent before exit: PPID=%ld Name=%s\n", (long)getppid(), parent_name);
        }

        sleep(2);
        parent_pid_after = (long)getppid();
        printf("[child] Parent after delay: PPID=%ld\n", parent_pid_after);
        printf("[child] The parent may have exited and the child may have been reparented by the system.\n");
        _exit(0);
    }

    sleep(1);
    printf("Parent PID %ld exiting after spawning child PID %ld.\n", (long)getpid(), (long)pid);
    _exit(0);
}

int cmd_signal_demo(int argc, char **argv) {
    static const struct {
        const char *name;
        int sig;
    } known_signals[] = {
        {"SIGINT", SIGINT},
        {"SIGTERM", SIGTERM},
        {"SIGUSR1", SIGUSR1},
        {"SIGUSR2", SIGUSR2},
        {"SIGCHLD", SIGCHLD},
        {NULL, 0}
    };

    pid_t target = g_created_pid;
    int sig = 0;
    int idx = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: signal <SIGINT|SIGTERM|SIGUSR1|SIGUSR2|SIGCHLD>\n");
        return -1;
    }

    if (target <= 0) {
        fprintf(stderr, "ERROR: no managed child process exists. Use 'create' first.\n");
        return -1;
    }

    for (idx = 0; known_signals[idx].name != NULL; idx++) {
        if (strcasecmp(argv[1], known_signals[idx].name) == 0) {
            sig = known_signals[idx].sig;
            break;
        }
    }

    if (sig == 0) {
        fprintf(stderr, "ERROR: signal '%s' is not supported in this console.\n", argv[1]);
        return -1;
    }

    if (kill(target, sig) != 0) {
        perror("kill");
        return -1;
    }

    printf("Signal %s sent to managed PID %ld.\n", argv[1], (long)target);
    return 0;
}

int cmd_execute(int argc, char **argv) {
    pid_t child;
    const char *command = NULL;
    int i;
    int status = 0;

    if (argc < 2) {
        fprintf(stderr, "Usage: execute <command> [args...]\n");
        return -1;
    }

    command = argv[1];
    child = fork();
    if (child < 0) {
        perror("fork");
        return -1;
    }

    if (child == 0) {
        char **exec_argv = calloc((size_t)argc, sizeof(char *));
        if (exec_argv == NULL) {
            perror("calloc");
            _exit(1);
        }
        for (i = 1; i < argc; i++) {
            exec_argv[i - 1] = argv[i];
        }
        exec_argv[argc - 1] = NULL;
        execvp(command, exec_argv);
        perror("execvp");
        free(exec_argv);
        _exit(1);
    }

    printf("Parent PID  : %ld\n", (long)getpid());
    printf("Child PID   : %ld\n", (long)child);
    printf("Command     : %s\n", command);

    if (waitpid(child, &status, 0) == -1) {
        perror("waitpid");
        return -1;
    }

    if (WIFEXITED(status)) {
        printf("Exit status : %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Exit status : terminated by signal %d\n", WTERMSIG(status));
    } else {
        printf("Exit status : unknown\n");
    }
    return 0;
}
