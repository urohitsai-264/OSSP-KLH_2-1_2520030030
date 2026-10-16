#include <stdio.h>
#include <string.h>

int main(void)
{
    char line[256];

    printf("Skill 1 - line input\n> ");
    if (fgets(line, sizeof(line), stdin) == NULL) {
        fprintf(stderr, "No input received\n");
        return 1;
    }

    line[strcspn(line, "\n")] = '\0';
    printf("Input: %s\n", line);
    return 0;
}
