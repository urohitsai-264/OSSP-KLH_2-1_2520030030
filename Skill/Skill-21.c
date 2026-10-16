#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 64

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    printf("Skill 21 - foreground process group\ncommand> ");
    fflush(stdout);
    if (getline(&line, &capacity, stdin) < 0)
        return 1;

    char *argv[MAX_ARGS + 1];
    size_t argc = 0;
    char *save = NULL;
    for (char *word = strtok_r(line, " \t\r\n", &save); word != NULL;
         word = strtok_r(NULL, " \t\r\n", &save)) {
        if (argc == MAX_ARGS) {
            fprintf(stderr, "Too many arguments\n");
            free(line);
            return 1;
        }
        argv[argc++] = word;
    }
    if (argc == 0) {
        free(line);
        return 0;
    }
    argv[argc] = NULL;
    int interactive = isatty(STDIN_FILENO);
    pid_t shell_group = getpgrp();
    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        free(line);
        return 1;
    }
    if (child == 0) {
        (void)setpgid(0, 0);
        struct sigaction action = {0};
        action.sa_handler = SIG_DFL;
        sigemptyset(&action.sa_mask);
        (void)sigaction(SIGINT, &action, NULL);
        (void)sigaction(SIGTSTP, &action, NULL);
        execvp(argv[0], argv);
        perror(argv[0]);
        _exit(127);
    }
    (void)setpgid(child, child);
    if (interactive && tcsetpgrp(STDIN_FILENO, child) < 0)
        perror("tcsetpgrp child");
    int status;
    if (waitpid(child, &status, WUNTRACED) < 0)
        perror("waitpid");
    if (interactive && tcsetpgrp(STDIN_FILENO, shell_group) < 0)
        perror("tcsetpgrp shell");
    if (WIFSTOPPED(status))
        printf("Child stopped by signal %d\n", WSTOPSIG(status));
    free(line);
    return 0;
}
