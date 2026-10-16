#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char **history = NULL;
    size_t count = 0;
    size_t capacity = 0;
    char *line = NULL;
    size_t line_capacity = 0;

    while (printf("Skill 13> "), fflush(stdout), getline(&line, &line_capacity, stdin) >= 0) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strcmp(line, "exit") == 0)
            break;
        if (strcmp(line, "history") == 0) {
            for (size_t i = 0; i < count; ++i)
                printf("%zu  %s\n", i + 1, history[i]);
            continue;
        }
        if (strcmp(line, "!!") == 0) {
            if (count == 0)
                puts("No commands in history");
            else
                printf("Last command: %s\n", history[count - 1]);
            continue;
        }
        if (line[0] == '\0')
            continue;
        if (count == capacity) {
            size_t new_capacity = capacity == 0 ? 8 : capacity * 2;
            char **grown = realloc(history, new_capacity * sizeof(*history));
            if (grown == NULL) {
                perror("realloc");
                break;
            }
            history = grown;
            capacity = new_capacity;
        }
        history[count] = strdup(line);
        if (history[count] == NULL) {
            perror("strdup");
            break;
        }
        ++count;
    }

    for (size_t i = 0; i < count; ++i)
        free(history[i]);
    free(history);
    free(line);
    return 0;
}
