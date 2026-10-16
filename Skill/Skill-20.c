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
    struct sigaction ignore_action = {0};
    ignore_action.sa_handler = SIG_IGN;
    sigemptyset(&ignore_action.sa_mask);
    if (sigaction(SIGINT, &ignore_action, NULL) < 0) {
        perror("sigaction");
        return 1;
    }

    char *line = NULL;
    size_t capacity = 0;
    while (printf("Skill 20 (Ctrl-C is ignored by parent)> "), fflush(stdout),
           getline(&line, &capacity, stdin) >= 0) {
        char *argv[MAX_ARGS + 1];
        size_t argc = 0;
        char *save = NULL;
        for (char *word = strtok_r(line, " \t\r\n", &save); word != NULL;
             word = strtok_r(NULL, " \t\r\n", &save)) {
            if (argc == MAX_ARGS) {
                fprintf(stderr, "Too many arguments\n");
                argc = 0;
                break;
            }
            argv[argc++] = word;
        }
        if (argc == 0)
            continue;
        if (strcmp(argv[0], "exit") == 0)
            break;
        argv[argc] = NULL;
        pid_t child = fork();
        if (child < 0) {
            perror("fork");
            continue;
        }
        if (child == 0) {
            struct sigaction default_action = {0};
            default_action.sa_handler = SIG_DFL;
            sigemptyset(&default_action.sa_mask);
            (void)sigaction(SIGINT, &default_action, NULL);
            execvp(argv[0], argv);
            perror(argv[0]);
            _exit(127);
        }
        (void)waitpid(child, NULL, 0);
    }
    free(line);
    return 0;
}
