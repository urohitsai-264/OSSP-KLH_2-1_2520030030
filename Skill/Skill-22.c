#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
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

static Job jobs[MAX_JOBS];

static Job *find_job(pid_t pid)
{
    for (size_t i = 0; i < MAX_JOBS; ++i)
        if (jobs[i].active && jobs[i].pid == pid)
            return &jobs[i];
    return NULL;
}

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    while (printf("Skill 22> "), fflush(stdout), getline(&line, &capacity, stdin) >= 0) {
        for (size_t i = 0; i < MAX_JOBS; ++i) {
            if (!jobs[i].active)
                continue;
            int status;
            pid_t done = waitpid(jobs[i].pid, &status, WNOHANG | WUNTRACED);
            if (done == jobs[i].pid && (WIFEXITED(status) || WIFSIGNALED(status)))
                jobs[i].active = 0;
        }
        char *argv[MAX_ARGS + 1];
        size_t argc = 0;
        char *save = NULL;
        for (char *word = strtok_r(line, " \t\r\n", &save); word != NULL;
             word = strtok_r(NULL, " \t\r\n", &save)) {
            if (argc < MAX_ARGS)
                argv[argc++] = word;
        }
        if (argc == 0)
            continue;
        if (strcmp(argv[0], "exit") == 0)
            break;
        if (strcmp(argv[0], "jobs") == 0) {
            for (size_t i = 0; i < MAX_JOBS; ++i)
                if (jobs[i].active)
                    printf("[%ld] %s\n", (long)jobs[i].pid, jobs[i].command);
            continue;
        }
        if ((strcmp(argv[0], "fg") == 0 || strcmp(argv[0], "bg") == 0) && argc == 2) {
            pid_t pid = (pid_t)strtol(argv[1], NULL, 10);
            Job *job = find_job(pid);
            if (job == NULL) {
                fprintf(stderr, "No such job: %ld\n", (long)pid);
                continue;
            }
            (void)kill(-pid, SIGCONT);
            if (strcmp(argv[0], "fg") == 0) {
                int status;
                if (waitpid(pid, &status, WUNTRACED) < 0)
                    perror("waitpid");
                else if (WIFEXITED(status) || WIFSIGNALED(status))
                    job->active = 0;
            }
            continue;
        }
        int background = strcmp(argv[argc - 1], "&") == 0;
        if (background)
            argv[--argc] = NULL;
        else
            argv[argc] = NULL;
        if (argc == 0)
            continue;
        pid_t child = fork();
        if (child < 0) {
            perror("fork");
            continue;
        }
        if (child == 0) {
            (void)setpgid(0, 0);
            execvp(argv[0], argv);
            perror(argv[0]);
            _exit(127);
        }
        (void)setpgid(child, child);
        if (!background) {
            int status;
            (void)waitpid(child, &status, WUNTRACED);
            if (WIFSTOPPED(status)) {
                size_t slot;
                for (slot = 0; slot < MAX_JOBS && jobs[slot].active; ++slot)
                    ;
                if (slot < MAX_JOBS) {
                    jobs[slot].pid = child;
                    jobs[slot].active = 1;
                    (void)snprintf(jobs[slot].command, sizeof(jobs[slot].command), "%s", argv[0]);
                }
            }
            continue;
        }
        size_t slot;
        for (slot = 0; slot < MAX_JOBS && jobs[slot].active; ++slot)
            ;
        if (slot == MAX_JOBS) {
            fprintf(stderr, "Job table full\n");
            (void)waitpid(child, NULL, 0);
        } else {
            jobs[slot].pid = child;
            jobs[slot].active = 1;
            (void)snprintf(jobs[slot].command, sizeof(jobs[slot].command), "%s", argv[0]);
            printf("[%ld] started\n", (long)child);
        }
    }
    for (size_t i = 0; i < MAX_JOBS; ++i)
        if (jobs[i].active)
            (void)kill(-jobs[i].pid, SIGTERM);
    for (size_t i = 0; i < MAX_JOBS; ++i)
        if (jobs[i].active)
            (void)waitpid(jobs[i].pid, NULL, 0);
    free(line);
    return 0;
}
