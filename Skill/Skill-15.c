#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_COMMANDS 8
#define MAX_ARGS 32

int main(void)
{
    char line[1024];
    char *commands[MAX_COMMANDS];
    char **arguments[MAX_COMMANDS];
    size_t command_count = 0;
    char *save_commands = NULL;

    printf("Skill 15 - pipeline (up to %d commands)\n> ", MAX_COMMANDS);
    if (fgets(line, sizeof(line), stdin) == NULL)
        return 1;
    for (char *part = strtok_r(line, "|", &save_commands); part != NULL;
         part = strtok_r(NULL, "|", &save_commands)) {
        if (command_count == MAX_COMMANDS) {
            fprintf(stderr, "Pipeline is too long\n");
            return 1;
        }
        commands[command_count++] = part;
    }
    if (command_count == 0) {
        fprintf(stderr, "Empty pipeline\n");
        return 1;
    }
    for (size_t i = 0; i < command_count; ++i) {
        arguments[i] = calloc(MAX_ARGS + 1, sizeof(*arguments[i]));
        if (arguments[i] == NULL) {
            perror("calloc");
            return 1;
        }
        char *save_words = NULL;
        char *word = strtok_r(commands[i], " \t\r\n", &save_words);
        size_t count = 0;
        while (word != NULL && count < MAX_ARGS) {
            arguments[i][count++] = word;
            word = strtok_r(NULL, " \t\r\n", &save_words);
        }
        if (count == 0 || word != NULL) {
            fprintf(stderr, "Empty or oversized command in pipeline\n");
            return 1;
        }
    }

    int previous_read = -1;
    pid_t children[MAX_COMMANDS];
    for (size_t i = 0; i < command_count; ++i) {
        int next_pipe[2] = {-1, -1};
        if (i + 1 < command_count && pipe(next_pipe) < 0) {
            perror("pipe");
            return 1;
        }
        pid_t child = fork();
        if (child < 0) {
            perror("fork");
            return 1;
        }
        if (child == 0) {
            if (previous_read >= 0) {
                (void)dup2(previous_read, STDIN_FILENO);
                close(previous_read);
            }
            if (next_pipe[1] >= 0) {
                (void)dup2(next_pipe[1], STDOUT_FILENO);
                close(next_pipe[1]);
                close(next_pipe[0]);
            }
            execvp(arguments[i][0], arguments[i]);
            perror(arguments[i][0]);
            _exit(127);
        }
        children[i] = child;
        if (previous_read >= 0)
            close(previous_read);
        if (next_pipe[1] >= 0)
            close(next_pipe[1]);
        previous_read = next_pipe[0];
    }
    if (previous_read >= 0)
        close(previous_read);
    for (size_t i = 0; i < command_count; ++i) {
        (void)waitpid(children[i], NULL, 0);
        free(arguments[i]);
    }
    return 0;
}
