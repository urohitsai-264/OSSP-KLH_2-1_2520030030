#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 32

static size_t split_words(char *text, char *argv[MAX_ARGS + 1])
{
    size_t count = 0;
    char *save = NULL;
    char *word = strtok_r(text, " \t\r\n", &save);
    while (word != NULL && count < MAX_ARGS) {
        argv[count++] = word;
        word = strtok_r(NULL, " \t\r\n", &save);
    }
    argv[count] = NULL;
    return word == NULL ? count : 0;
}

int main(void)
{
    char line[512];
    printf("Skill 14 - one pipe (left | right)\n> ");
    if (fgets(line, sizeof(line), stdin) == NULL)
        return 1;
    char *separator = strchr(line, '|');
    if (separator == NULL) {
        fprintf(stderr, "Expected one | operator\n");
        return 1;
    }
    *separator++ = '\0';
    char *left[MAX_ARGS + 1];
    char *right[MAX_ARGS + 1];
    if (split_words(line, left) == 0 || split_words(separator, right) == 0) {
        fprintf(stderr, "Empty or oversized pipeline command\n");
        return 1;
    }
    int descriptors[2];
    if (pipe(descriptors) < 0) {
        perror("pipe");
        return 1;
    }
    pid_t first = fork();
    if (first == 0) {
        close(descriptors[0]);
        if (dup2(descriptors[1], STDOUT_FILENO) < 0) {
            perror("dup2");
            _exit(126);
        }
        close(descriptors[1]);
        execvp(left[0], left);
        perror(left[0]);
        _exit(127);
    }
    if (first < 0) {
        perror("fork");
        return 1;
    }
    pid_t second = fork();
    if (second == 0) {
        close(descriptors[1]);
        if (dup2(descriptors[0], STDIN_FILENO) < 0) {
            perror("dup2");
            _exit(126);
        }
        close(descriptors[0]);
        execvp(right[0], right);
        perror(right[0]);
        _exit(127);
    }
    close(descriptors[0]);
    close(descriptors[1]);
    if (second < 0) {
        perror("fork");
        (void)waitpid(first, NULL, 0);
        return 1;
    }
    (void)waitpid(first, NULL, 0);
    (void)waitpid(second, NULL, 0);
    return 0;
}
