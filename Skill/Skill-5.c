#include <stdio.h>
#include <string.h>

#define MAX_ARGS 32

int main(void)
{
    char line[512];
    char *token;
    size_t count = 0;

    printf("Skill 5 - command validation\ncommand> ");
    if (fgets(line, sizeof(line), stdin) == NULL)
        return 1;
    if (strchr(line, '\n') == NULL && !feof(stdin)) {
        fprintf(stderr, "Input line is too long\n");
        return 1;
    }

    token = strtok(line, " \t\r\n");
    while (token != NULL) {
        if (count == MAX_ARGS) {
            fprintf(stderr, "Too many arguments\n");
            return 1;
        }
        ++count;
        token = strtok(NULL, " \t\r\n");
    }
    if (count == 0) {
        fprintf(stderr, "Empty command\n");
        return 1;
    }
    printf("Valid command with %zu argument(s)\n", count);
    return 0;
}
