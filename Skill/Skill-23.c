#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 64
#define MAX_STAGES 8

typedef struct {
    char *argv[MAX_ARGS + 1];
    char *input;
    char *output;
    int append;
} Stage;

static int parse_stage(char *text, Stage *stage)
{
    size_t argc = 0;
    char *save = NULL;
    for (char *word = strtok_r(text, " \t\r\n", &save); word != NULL;
         word = strtok_r(NULL, " \t\r\n", &save)) {
        if (strcmp(word, "<") == 0 || strcmp(word, ">") == 0 || strcmp(word, ">>") == 0) {
            int is_input = word[0] == '<';
            char *path = strtok_r(NULL, " \t\r\n", &save);
            if (path == NULL)
                return -1;
            if (is_input)
                stage->input = path;
            else {
                stage->output = path;
                stage->append = strcmp(word, ">>") == 0;
            }
        } else if (argc < MAX_ARGS) {
            stage->argv[argc++] = word;
        } else {
            return -1;
        }
    }
    stage->argv[argc] = NULL;
    return argc == 0 ? -1 : 0;
}

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    while (printf("Skill 23> "), fflush(stdout), getline(&line, &capacity, stdin) >= 0) {
        char *amp = strrchr(line, '&');
        int background = amp != NULL;
        if (background)
            *amp = '\0';
        Stage stages[MAX_STAGES] = {0};
        size_t count = 0;
        char *save = NULL;
        for (char *part = strtok_r(line, "|", &save); part != NULL;
             part = strtok_r(NULL, "|", &save)) {
            if (count == MAX_STAGES || parse_stage(part, &stages[count]) < 0) {
                fprintf(stderr, "Invalid or oversized command pipeline\n");
                count = 0;
                break;
            }
            ++count;
        }
        if (count == 0)
            continue;
        pid_t pids[MAX_STAGES];
        int previous = -1;
        for (size_t i = 0; i < count; ++i) {
            int next[2] = {-1, -1};
            if (i + 1 < count && pipe(next) < 0) {
                perror("pipe");
                break;
            }
            pid_t child = fork();
            if (child < 0) {
                perror("fork");
                break;
            }
            if (child == 0) {
                if (previous >= 0)
                    (void)dup2(previous, STDIN_FILENO);
                if (next[1] >= 0)
                    (void)dup2(next[1], STDOUT_FILENO);
                if (previous >= 0)
                    close(previous);
                if (next[0] >= 0)
                    close(next[0]);
                if (next[1] >= 0)
                    close(next[1]);
                if (stages[i].input != NULL) {
                    int fd = open(stages[i].input, O_RDONLY);
                    if (fd < 0 || dup2(fd, STDIN_FILENO) < 0) {
                        perror(stages[i].input);
                        _exit(126);
                    }
                    close(fd);
                }
                if (stages[i].output != NULL) {
                    int flags = O_WRONLY | O_CREAT | (stages[i].append ? O_APPEND : O_TRUNC);
                    int fd = open(stages[i].output, flags, 0666);
                    if (fd < 0 || dup2(fd, STDOUT_FILENO) < 0) {
                        perror(stages[i].output);
                        _exit(126);
                    }
                    close(fd);
                }
                execvp(stages[i].argv[0], stages[i].argv);
                perror(stages[i].argv[0]);
                _exit(127);
            }
            pids[i] = child;
            if (previous >= 0)
                close(previous);
            if (next[1] >= 0)
                close(next[1]);
            previous = next[0];
        }
        if (previous >= 0)
            close(previous);
        if (background)
            printf("started background pipeline pid %ld\n", (long)pids[count - 1]);
        else
            for (size_t i = 0; i < count; ++i)
                (void)waitpid(pids[i], NULL, 0);
    }
    free(line);
    return 0;
}
