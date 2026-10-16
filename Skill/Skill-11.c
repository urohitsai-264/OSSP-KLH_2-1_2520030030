#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;

    while (printf("Skill 11> "), fflush(stdout), getline(&line, &capacity, stdin) >= 0) {
        char *save = NULL;
        char *command = strtok_r(line, " \t\r\n", &save);
        char *argument = strtok_r(NULL, " \t\r\n", &save);
        if (command == NULL)
            continue;
        if (strcmp(command, "exit") == 0)
            break;
        if (strcmp(command, "pwd") == 0) {
            char cwd[4096];
            if (getcwd(cwd, sizeof(cwd)) == NULL)
                perror("getcwd");
            else
                puts(cwd);
        } else if (strcmp(command, "cd") == 0) {
            const char *directory = argument == NULL ? getenv("HOME") : argument;
            if (directory == NULL || chdir(directory) < 0)
                perror("cd");
        } else {
            fprintf(stderr, "Built-ins: cd [dir], pwd, exit\n");
        }
    }
    free(line);
    return 0;
}
