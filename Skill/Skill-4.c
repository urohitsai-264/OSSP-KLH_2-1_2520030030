#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>

#define MAX_ARGS 32

int main(void)
{
    char line[512];
    char *save = NULL;
    char *token;
    size_t count = 0;

    printf("Skill 4 - whitespace tokens\ncommand> ");
    if (fgets(line, sizeof(line), stdin) == NULL)
        return 1;

    for (token = strtok_r(line, " \t\r\n", &save); token != NULL;
         token = strtok_r(NULL, " \t\r\n", &save)) {
        if (count == MAX_ARGS) {
            fprintf(stderr, "Too many tokens (limit %d)\n", MAX_ARGS);
            return 1;
        }
        printf("argv[%zu] = <%s>\n", count++, token);
    }
    printf("argc = %zu\n", count);
    return 0;
}
