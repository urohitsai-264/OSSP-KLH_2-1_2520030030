#include <stdio.h>
#include <string.h>

#define LINE_SIZE 512
#define MAX_ARGS 32
#define ARG_SIZE 128

int main(void)
{
    char line[LINE_SIZE];
    char args[MAX_ARGS][ARG_SIZE];
    size_t argc = 0;
    size_t used = 0;
    char quote = '\0';
    int escaped = 0;
    int active = 0;

    printf("Skill 6 - quotes and escapes\ncommand> ");
    if (fgets(line, sizeof(line), stdin) == NULL)
        return 1;

    for (size_t i = 0; ; ++i) {
        char ch = line[i];
        if (escaped) {
            if (ch == '\0' || ch == '\n') {
                fprintf(stderr, "Trailing escape\n");
                return 1;
            }
            if (used + 1 >= ARG_SIZE) {
                fprintf(stderr, "Argument too long\n");
                return 1;
            }
            args[argc][used++] = ch;
            escaped = 0;
            active = 1;
            continue;
        }
        if (ch == '\\' && quote != '\'') {
            escaped = 1;
            active = 1;
            continue;
        }
        if ((ch == '\'' || ch == '"') && (quote == '\0' || quote == ch)) {
            quote = quote == '\0' ? ch : '\0';
            active = 1;
            continue;
        }
        if (ch == '\0' || ch == '\n' || (quote == '\0' && (ch == ' ' || ch == '\t'))) {
            if (quote != '\0') {
                fprintf(stderr, "Unclosed quote\n");
                return 1;
            }
            if (active) {
                args[argc][used] = '\0';
                ++argc;
                used = 0;
                active = 0;
                if (argc == MAX_ARGS)
                    break;
            }
            if (ch == '\0' || ch == '\n')
                break;
            continue;
        }
        if (argc == MAX_ARGS || used + 1 >= ARG_SIZE) {
            fprintf(stderr, "Command exceeds parser limits\n");
            return 1;
        }
        args[argc][used++] = ch;
        active = 1;
    }

    for (size_t i = 0; i < argc; ++i)
        printf("argv[%zu] = <%s>\n", i, args[i]);
    printf("argc = %zu\n", argc);
    return 0;
}
