#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char line[512];
    char *save = NULL;
    char *token;

    printf("Skill 10 - expand $NAME\ncommand> ");
    if (fgets(line, sizeof(line), stdin) == NULL)
        return 1;
    for (token = strtok_r(line, " \t\r\n", &save); token != NULL;
         token = strtok_r(NULL, " \t\r\n", &save)) {
        if (token[0] == '$' && token[1] != '\0') {
            const char *value = getenv(token + 1);
            printf("%s\n", value == NULL ? "" : value);
        } else {
            printf("%s\n", token);
        }
    }
    return 0;
}
