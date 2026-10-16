#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
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
    char *argv[MAX_ARGS + 1];
    size_t argc = 0;
    char *output_path = NULL;
    char *error_path = NULL;
    int append_output = 0;
    char *save = NULL;

    printf("Skill 17 - >> append and 2> errors\n> ");
    fflush(stdout);
    if (getline(&line, &capacity, stdin) < 0)
        return 1;
    for (char *token = strtok_r(line, " \t\r\n", &save); token != NULL;
         token = strtok_r(NULL, " \t\r\n", &save)) {
        if (strcmp(token, ">") == 0 || strcmp(token, ">>") == 0 || strcmp(token, "2>") == 0) {
            char *path = strtok_r(NULL, " \t\r\n", &save);
            int is_error = strcmp(token, "2>") == 0;
            if (path == NULL || (is_error ? error_path != NULL : output_path != NULL)) {
                fprintf(stderr, "Invalid or repeated redirection\n");
                free(line);
                return 1;
            }
            if (is_error)
                error_path = path;
            else {
                output_path = path;
                append_output = strcmp(token, ">>") == 0;
            }
        } else if (argc < MAX_ARGS) {
            argv[argc++] = token;
        } else {
            fprintf(stderr, "Too many arguments\n");
            free(line);
            return 1;
        }
    }
    argv[argc] = NULL;
    if (argc == 0) {
        fprintf(stderr, "Missing command\n");
        free(line);
        return 1;
    }

    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        free(line);
        return 1;
    }
    if (child == 0) {
        if (output_path != NULL) {
            int flags = O_WRONLY | O_CREAT | (append_output ? O_APPEND : O_TRUNC);
            int fd = open(output_path, flags, 0666);
            if (fd < 0 || dup2(fd, STDOUT_FILENO) < 0) {
                perror(output_path);
                _exit(126);
            }
            close(fd);
        }
        if (error_path != NULL) {
            int fd = open(error_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd < 0 || dup2(fd, STDERR_FILENO) < 0) {
                perror(error_path);
                _exit(126);
            }
            close(fd);
        }
        execvp(argv[0], argv);
        perror(argv[0]);
        _exit(127);
    }
    int status;
    if (waitpid(child, &status, 0) < 0) {
        perror("waitpid");
        free(line);
        return 1;
    }
    free(line);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
