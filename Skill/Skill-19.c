#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_JOBS 32
#define MAX_ARGS 64

typedef struct {
    pid_t pid;
    char command[128];
    int active;
} Job;

static void reap_jobs(Job jobs[MAX_JOBS])
{
    for (size_t i = 0; i < MAX_JOBS; ++i) {
        if (!jobs[i].active)
            continue;
        int status;
        pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);
        if (result == jobs[i].pid) {
            printf("[%ld] done %s\n", (long)jobs[i].pid, jobs[i].command);
            jobs[i].active = 0;
        } else if (result < 0 && errno != EINTR) {
            jobs[i].active = 0;
        }
    }
}

int main(void)
{
    Job jobs[MAX_JOBS] = {0};
    char *line = NULL;
    size_t capacity = 0;
    while (printf("Skill 19> "), fflush(stdout), getline(&line, &capacity, stdin) >= 0) {
        reap_jobs(jobs);
        char *argv[MAX_ARGS + 1];
        size_t argc = 0;
        char *save = NULL;
        char *word = strtok_r(line, " \t\r\n", &save);
        int background = 0;
        while (word != NULL && argc < MAX_ARGS) {
            if (strcmp(word, "&") == 0)
                background = 1;
            else
                argv[argc++] = word;
            word = strtok_r(NULL, " \t\r\n", &save);
        }
        if (word != NULL) {
            fprintf(stderr, "Too many arguments\n");
            continue;
        }
        if (argc == 0)
            continue;
        if (strcmp(argv[0], "exit") == 0)
            break;
        if (strcmp(argv[0], "jobs") == 0) {
            for (size_t i = 0; i < MAX_JOBS; ++i)
                if (jobs[i].active)
                    printf("[%ld] running %s\n", (long)jobs[i].pid, jobs[i].command);
            continue;
        }
        argv[argc] = NULL;
        pid_t child = fork();
        if (child < 0) {
            perror("fork");
            continue;
        }
        if (child == 0) {
            execvp(argv[0], argv);
            perror(argv[0]);
            _exit(127);
        }
        if (!background) {
            (void)waitpid(child, NULL, 0);
            continue;
        }
        size_t slot;
        for (slot = 0; slot < MAX_JOBS && jobs[slot].active; ++slot)
            ;
        if (slot == MAX_JOBS) {
            fprintf(stderr, "Job table full; waiting for process\n");
            (void)waitpid(child, NULL, 0);
        } else {
            jobs[slot].pid = child;
            jobs[slot].active = 1;
            (void)snprintf(jobs[slot].command, sizeof(jobs[slot].command), "%s", argv[0]);
            printf("[%ld] started %s\n", (long)child, argv[0]);
        }
    }
    for (size_t i = 0; i < MAX_JOBS; ++i)
        if (jobs[i].active)
            (void)waitpid(jobs[i].pid, NULL, 0);
    free(line);
    return 0;
}
