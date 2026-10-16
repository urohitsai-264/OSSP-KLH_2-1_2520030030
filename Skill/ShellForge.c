#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_TOKENS 128
#define TOKEN_SIZE 1024
#define MAX_ARGS 64
#define MAX_STAGES 8
#define MAX_JOBS 32
#define MAX_HISTORY 256

typedef struct {
    char text[TOKEN_SIZE];
    int is_operator;
} Token;

typedef struct {
    char *argv[MAX_ARGS + 1];
    size_t argc;
    char *input;
    char *output;
    char *error;
    int append;
} Command;

typedef struct {
    Command commands[MAX_STAGES];
    size_t count;
    int background;
} Pipeline;

typedef struct {
    int active;
    int stopped;
    int remaining;
    int job_id;
    pid_t pgid;
    pid_t last_pid;
    int exit_status;
    char command[256];
} Job;

static Job jobs[MAX_JOBS];
static char *history[MAX_HISTORY];
static size_t history_count;
static int next_job_id = 1;
static int interactive;
static pid_t shell_pgid;
static int last_status;
static int should_exit;
static int requested_status;

static int append_char(char *buffer, size_t *length, char ch)
{
    if (*length + 1 >= TOKEN_SIZE)
        return -1;
    buffer[(*length)++] = ch;
    buffer[*length] = '\0';
    return 0;
}

static int append_text(char *buffer, size_t *length, const char *text)
{
    while (*text != '\0') {
        if (append_char(buffer, length, *text++) < 0)
            return -1;
    }
    return 0;
}

static int expand_variable(const char *line, size_t *index, char *buffer, size_t *length)
{
    char name[128];
    size_t name_length = 0;
    const char *value;

    ++*index;
    if (line[*index] == '?') {
        char status_text[32];
        (void)snprintf(status_text, sizeof(status_text), "%d", last_status);
        ++*index;
        return append_text(buffer, length, status_text);
    }
    while ((isalnum((unsigned char)line[*index]) || line[*index] == '_') &&
           name_length + 1 < sizeof(name))
        name[name_length++] = line[(*index)++];
    if (name_length == 0)
        return append_char(buffer, length, '$');
    name[name_length] = '\0';
    value = getenv(name);
    return value == NULL ? 0 : append_text(buffer, length, value);
}

static int add_operator(Token tokens[MAX_TOKENS], size_t *count, const char *text)
{
    if (*count >= MAX_TOKENS)
        return -1;
    (void)snprintf(tokens[*count].text, sizeof(tokens[*count].text), "%s", text);
    tokens[*count].is_operator = 1;
    ++*count;
    return 0;
}

static int tokenize(const char *line, Token tokens[MAX_TOKENS], size_t *token_count)
{
    size_t i = 0;
    *token_count = 0;
    while (line[i] != '\0') {
        while (line[i] == ' ' || line[i] == '\t' || line[i] == '\r' || line[i] == '\n')
            ++i;
        if (line[i] == '\0')
            break;
        if (line[i] == '|' || line[i] == '&' || line[i] == '<' || line[i] == '>' ||
            (line[i] == '2' && line[i + 1] == '>')) {
            char op[3] = {line[i], '\0', '\0'};
            if (line[i] == '2') {
                op[1] = '>';
                i += 2;
            } else {
                ++i;
                if (op[0] == '>' && line[i] == '>') {
                    op[1] = '>';
                    ++i;
                }
            }
            if (add_operator(tokens, token_count, op) < 0)
                return -1;
            continue;
        }
        if (*token_count >= MAX_TOKENS)
            return -1;
        Token *token = &tokens[(*token_count)++];
        size_t used = 0;
        char quote = '\0';
        int active = 0;
        token->text[0] = '\0';
        token->is_operator = 0;
        while (line[i] != '\0') {
            char ch = line[i];
            if (quote == '\0' && (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' ||
                                   ch == '|' || ch == '&' || ch == '<' || ch == '>'))
                break;
            if (ch == '\'' && quote != '"') {
                quote = quote == '\'' ? '\0' : '\'';
                active = 1;
                ++i;
                continue;
            }
            if (ch == '"' && quote != '\'') {
                quote = quote == '"' ? '\0' : '"';
                active = 1;
                ++i;
                continue;
            }
            if (ch == '\\' && quote != '\'') {
                ++i;
                if (line[i] == '\0')
                    return -1;
                if (append_char(token->text, &used, line[i++]) < 0)
                    return -1;
                active = 1;
                continue;
            }
            if (ch == '$' && quote != '\'') {
                if (expand_variable(line, &i, token->text, &used) < 0)
                    return -1;
                active = 1;
                continue;
            }
            if (append_char(token->text, &used, ch) < 0)
                return -1;
            active = 1;
            ++i;
        }
        if (quote != '\0' || !active)
            return -1;
    }
    return 0;
}

static int parse_pipeline(Token tokens[MAX_TOKENS], size_t token_count, Pipeline *pipeline)
{
    memset(pipeline, 0, sizeof(*pipeline));
    pipeline->count = 1;
    Command *command = &pipeline->commands[0];
    for (size_t i = 0; i < token_count; ++i) {
        Token *token = &tokens[i];
        if (token->is_operator && strcmp(token->text, "|") == 0) {
            if (command->argc == 0 || pipeline->count == MAX_STAGES)
                return -1;
            command = &pipeline->commands[pipeline->count++];
            continue;
        }
        if (token->is_operator && strcmp(token->text, "&") == 0) {
            if (i + 1 != token_count || command->argc == 0)
                return -1;
            pipeline->background = 1;
            continue;
        }
        if (token->is_operator && (strcmp(token->text, "<") == 0 ||
                                   strcmp(token->text, ">") == 0 ||
                                   strcmp(token->text, ">>") == 0 ||
                                   strcmp(token->text, "2>") == 0)) {
            if (++i >= token_count || tokens[i].is_operator)
                return -1;
            char **target;
            if (strcmp(token->text, "<") == 0)
                target = &command->input;
            else if (strcmp(token->text, "2>") == 0)
                target = &command->error;
            else {
                target = &command->output;
                command->append = strcmp(token->text, ">>") == 0;
            }
            if (*target != NULL)
                return -1;
            *target = tokens[i].text;
            continue;
        }
        if (command->argc == MAX_ARGS)
            return -1;
        command->argv[command->argc++] = token->text;
    }
    for (size_t i = 0; i < pipeline->count; ++i) {
        if (pipeline->commands[i].argc == 0)
            return -1;
        pipeline->commands[i].argv[pipeline->commands[i].argc] = NULL;
    }
    return 0;
}

static void add_history(const char *line)
{
    if (line[0] == '\0')
        return;
    if (history_count == MAX_HISTORY) {
        free(history[0]);
        memmove(history, history + 1, (MAX_HISTORY - 1) * sizeof(*history));
        --history_count;
    }
    history[history_count] = strdup(line);
    if (history[history_count] != NULL)
        ++history_count;
}

static void reap_jobs(void)
{
    for (size_t i = 0; i < MAX_JOBS; ++i) {
        Job *job = &jobs[i];
        if (!job->active)
            continue;
        int status;
        pid_t result;
        while ((result = waitpid(-job->pgid, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
            if (WIFSTOPPED(status)) {
                job->stopped = 1;
                printf("[%d] stopped %s\n", job->job_id, job->command);
            } else if (WIFCONTINUED(status)) {
                job->stopped = 0;
            } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                --job->remaining;
            }
        }
        if (job->remaining <= 0) {
            printf("[%d] done %s\n", job->job_id, job->command);
            memset(job, 0, sizeof(*job));
        }
    }
}

static Job *find_job(const char *text)
{
    int id = 0;
    if (text != NULL) {
        if (*text == '%')
            ++text;
        char *end = NULL;
        long parsed = strtol(text, &end, 10);
        if (end == text || *end != '\0' || parsed <= 0)
            return NULL;
        id = (int)parsed;
    }
    Job *latest = NULL;
    for (size_t i = 0; i < MAX_JOBS; ++i) {
        if (!jobs[i].active)
            continue;
        if (text != NULL && jobs[i].job_id == id)
            return &jobs[i];
        if (text == NULL && (latest == NULL || jobs[i].job_id > latest->job_id))
            latest = &jobs[i];
    }
    return latest;
}

static void give_terminal(pid_t group)
{
    if (interactive && tcsetpgrp(STDIN_FILENO, group) < 0)
        perror("tcsetpgrp");
}

static int wait_for_job(Job *job)
{
    job->stopped = 0;
    while (job->remaining > 0) {
        int status;
        pid_t result = waitpid(-job->pgid, &status, WUNTRACED);
        if (result < 0) {
            if (errno == EINTR)
                continue;
            if (errno == ECHILD)
                job->remaining = 0;
            else
                perror("waitpid");
            break;
        }
        if (result == job->last_pid) {
            if (WIFEXITED(status))
                job->exit_status = WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                job->exit_status = 128 + WTERMSIG(status);
            else if (WIFSTOPPED(status))
                job->exit_status = 128 + WSTOPSIG(status);
        }
        if (WIFSTOPPED(status)) {
            job->stopped = 1;
            (void)kill(-job->pgid, SIGSTOP);
            break;
        }
        if (WIFEXITED(status) || WIFSIGNALED(status))
            --job->remaining;
    }
    return job->exit_status;
}

static int run_builtin(Command *command)
{
    char **argv = command->argv;
    if (strcmp(argv[0], "cd") == 0) {
        const char *target = argv[1] == NULL ? getenv("HOME") : argv[1];
        char old_directory[4096];
        if (target == NULL || getcwd(old_directory, sizeof(old_directory)) == NULL ||
            chdir(target) < 0) {
            perror("cd");
            return 1;
        }
        (void)setenv("OLDPWD", old_directory, 1);
        char new_directory[4096];
        if (getcwd(new_directory, sizeof(new_directory)) != NULL)
            (void)setenv("PWD", new_directory, 1);
        return 0;
    }
    if (strcmp(argv[0], "pwd") == 0) {
        char directory[4096];
        if (getcwd(directory, sizeof(directory)) == NULL) {
            perror("getcwd");
            return 1;
        }
        puts(directory);
        return 0;
    }
    if (strcmp(argv[0], "export") == 0) {
        if (argv[1] == NULL) {
            fprintf(stderr, "Usage: export NAME=VALUE\n");
            return 2;
        }
        char *equals = strchr(argv[1], '=');
        if (equals == NULL || equals == argv[1]) {
            fprintf(stderr, "Usage: export NAME=VALUE\n");
            return 2;
        }
        *equals = '\0';
        if (setenv(argv[1], equals + 1, 1) < 0) {
            perror("export");
            return 1;
        }
        return 0;
    }
    if (strcmp(argv[0], "history") == 0) {
        for (size_t i = 0; i < history_count; ++i)
            printf("%zu  %s\n", i + 1, history[i]);
        return 0;
    }
    if (strcmp(argv[0], "jobs") == 0) {
        reap_jobs();
        for (size_t i = 0; i < MAX_JOBS; ++i)
            if (jobs[i].active)
                printf("[%d] %s %s\n", jobs[i].job_id,
                       jobs[i].stopped ? "stopped" : "running", jobs[i].command);
        return 0;
    }
    if (strcmp(argv[0], "fg") == 0 || strcmp(argv[0], "bg") == 0) {
        int foreground = strcmp(argv[0], "fg") == 0;
        Job *job = find_job(argv[1]);
        if (job == NULL) {
            fprintf(stderr, "No such job\n");
            return 1;
        }
        if (foreground) {
            give_terminal(job->pgid);
            (void)kill(-job->pgid, SIGCONT);
            int status = wait_for_job(job);
            give_terminal(shell_pgid);
            if (job->remaining <= 0)
                memset(job, 0, sizeof(*job));
            return status;
        } else {
            (void)kill(-job->pgid, SIGCONT);
            job->stopped = 0;
            printf("[%d] continued %s\n", job->job_id, job->command);
        }
        return 0;
    }
    if (strcmp(argv[0], "exit") == 0) {
        should_exit = 1;
        requested_status = argv[1] == NULL ? last_status : atoi(argv[1]);
        return 0;
    }
    if (strcmp(argv[0], "help") == 0) {
        puts("Built-ins: cd pwd export history jobs fg [job] bg [job] help exit");
        puts("External commands support quotes, $NAME, pipelines, <, >, >>, 2>, and &.");
        return 0;
    }
    return -1;
}

static int apply_redirection(const char *path, int target, int flags)
{
    int descriptor = open(path, flags, 0666);
    if (descriptor < 0) {
        perror(path);
        return -1;
    }
    if (dup2(descriptor, target) < 0) {
        perror("dup2");
        close(descriptor);
        return -1;
    }
    close(descriptor);
    return 0;
}

static int launch_pipeline(Pipeline *pipeline, const char *line)
{
    Job *job = NULL;
    for (size_t i = 0; i < MAX_JOBS; ++i) {
        if (!jobs[i].active) {
            job = &jobs[i];
            break;
        }
    }
    if (job == NULL) {
        fprintf(stderr, "Job table full\n");
        return 1;
    }

    int pipes[MAX_STAGES - 1][2];
    for (size_t i = 0; i + 1 < pipeline->count; ++i) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            for (size_t j = 0; j < i; ++j) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return 1;
        }
    }

    memset(job, 0, sizeof(*job));
    job->active = 1;
    job->remaining = (int)pipeline->count;
    job->job_id = next_job_id++;
    (void)snprintf(job->command, sizeof(job->command), "%s", line);
    for (size_t i = 0; i < pipeline->count; ++i) {
        pid_t child = fork();
        if (child < 0) {
            perror("fork");
            if (job->pgid > 0)
                (void)kill(-job->pgid, SIGTERM);
            job->remaining = (int)i;
            break;
        }
        if (child == 0) {
            pid_t group = job->pgid == 0 ? 0 : job->pgid;
            (void)setpgid(0, group);
            struct sigaction action = {0};
            action.sa_handler = SIG_DFL;
            sigemptyset(&action.sa_mask);
            (void)sigaction(SIGINT, &action, NULL);
            (void)sigaction(SIGQUIT, &action, NULL);
            (void)sigaction(SIGTSTP, &action, NULL);
            (void)sigaction(SIGTTIN, &action, NULL);
            (void)sigaction(SIGTTOU, &action, NULL);

            if (i > 0 && dup2(pipes[i - 1][0], STDIN_FILENO) < 0) {
                perror("dup2 stdin");
                _exit(126);
            }
            if (i + 1 < pipeline->count && dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                perror("dup2 stdout");
                _exit(126);
            }
            for (size_t j = 0; j + 1 < pipeline->count; ++j) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            Command *command = &pipeline->commands[i];
            if (command->input != NULL && apply_redirection(command->input, STDIN_FILENO, O_RDONLY) < 0)
                _exit(126);
            if (command->output != NULL) {
                int flags = O_WRONLY | O_CREAT | (command->append ? O_APPEND : O_TRUNC);
                if (apply_redirection(command->output, STDOUT_FILENO, flags) < 0)
                    _exit(126);
            }
            if (command->error != NULL &&
                apply_redirection(command->error, STDERR_FILENO, O_WRONLY | O_CREAT | O_TRUNC) < 0)
                _exit(126);
            execvp(command->argv[0], command->argv);
            int exec_error = errno;
            perror(command->argv[0]);
            _exit(exec_error == ENOENT ? 127 : 126);
        }
        if (job->pgid == 0)
            job->pgid = child;
        if (i + 1 == pipeline->count)
            job->last_pid = child;
        (void)setpgid(child, job->pgid);
    }
    for (size_t i = 0; i + 1 < pipeline->count; ++i) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    if (job->pgid == 0) {
        memset(job, 0, sizeof(*job));
        return 1;
    }
    if (pipeline->background) {
        printf("[%d] %ld\n", job->job_id, (long)job->pgid);
        return 0;
    }
    give_terminal(job->pgid);
    int status = wait_for_job(job);
    give_terminal(shell_pgid);
    if (job->stopped)
        printf("[%d] stopped %s\n", job->job_id, job->command);
    else {
        int status = 0;
        (void)waitpid(-job->pgid, &status, WNOHANG);
        memset(job, 0, sizeof(*job));
    }
    return status;
}

static void initialize_shell(void)
{
    interactive = isatty(STDIN_FILENO);
    shell_pgid = getpgrp();
    if (!interactive)
        return;
    while (tcgetpgrp(STDIN_FILENO) != (shell_pgid = getpgrp()))
        (void)kill(-shell_pgid, SIGTTIN);
    struct sigaction ignore_action = {0};
    ignore_action.sa_handler = SIG_IGN;
    sigemptyset(&ignore_action.sa_mask);
    (void)sigaction(SIGINT, &ignore_action, NULL);
    (void)sigaction(SIGQUIT, &ignore_action, NULL);
    (void)sigaction(SIGTSTP, &ignore_action, NULL);
    (void)sigaction(SIGTTIN, &ignore_action, NULL);
    (void)sigaction(SIGTTOU, &ignore_action, NULL);
    shell_pgid = getpid();
    if (getpgrp() != shell_pgid && setpgid(shell_pgid, shell_pgid) < 0 && errno != EACCES)
        perror("setpgid");
    give_terminal(shell_pgid);
}

int main(void)
{
    initialize_shell();
    char *line = NULL;
    size_t capacity = 0;
    while (!should_exit) {
        reap_jobs();
        if (interactive) {
            printf("2520030030_ShellForge$ ");
            fflush(stdout);
        }
        ssize_t length = getline(&line, &capacity, stdin);
        if (length < 0)
            break;
        if (length > 0 && line[length - 1] == '\n')
            line[length - 1] = '\0';
        add_history(line);
        Token tokens[MAX_TOKENS] = {0};
        size_t token_count = 0;
        Pipeline pipeline;
        if (tokenize(line, tokens, &token_count) < 0 ||
            (token_count > 0 && parse_pipeline(tokens, token_count, &pipeline) < 0)) {
            fprintf(stderr, "ShellForge: invalid quoting, token, or command syntax\n");
            last_status = 2;
            continue;
        }
        if (token_count == 0)
            continue;
        if (pipeline.count == 1 && !pipeline.background) {
            int result = run_builtin(&pipeline.commands[0]);
            if (result >= 0) {
                last_status = result;
                continue;
            }
        }
        last_status = launch_pipeline(&pipeline, line);
    }

    for (size_t i = 0; i < MAX_JOBS; ++i) {
        if (jobs[i].active) {
            (void)kill(-jobs[i].pgid, SIGHUP);
            (void)kill(-jobs[i].pgid, SIGCONT);
        }
    }
    for (size_t i = 0; i < MAX_JOBS; ++i)
        if (jobs[i].active)
            (void)waitpid(-jobs[i].pgid, NULL, 0);
    for (size_t i = 0; i < history_count; ++i)
        free(history[i]);
    free(line);
    return should_exit ? requested_status : last_status;
}
