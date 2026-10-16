#define _POSIX_C_SOURCE 200809L
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
    while (printf("Skill 18> "), fflush(stdout), getline(&line, &capacity, stdin) >= 0) {
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
        if (background)
            printf("started background pid %ld\n", (long)child);
        else if (waitpid(child, NULL, 0) < 0)
            perror("waitpid");
    }
    free(line);
    return 0;
}
