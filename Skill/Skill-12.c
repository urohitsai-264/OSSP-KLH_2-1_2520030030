#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;

    while (printf("Skill 12> "), fflush(stdout), getline(&line, &capacity, stdin) >= 0) {
        char *save = NULL;
        char *command = strtok_r(line, " \t\r\n", &save);
        char *assignment = strtok_r(NULL, " \t\r\n", &save);
        char *equals;
        if (command == NULL)
            continue;
        if (strcmp(command, "exit") == 0)
            break;
        if (strcmp(command, "export") != 0 || assignment == NULL ||
            (equals = strchr(assignment, '=')) == NULL || equals == assignment) {
            fprintf(stderr, "Usage: export NAME=VALUE\n");
            continue;
        }
        *equals = '\0';
        if (setenv(assignment, equals + 1, 1) < 0)
            perror("setenv");
        else
            printf("%s=%s\n", assignment, getenv(assignment));
    }
    free(line);
    return 0;
}
