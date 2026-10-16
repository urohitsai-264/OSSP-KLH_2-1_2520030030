#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    pid_t child = fork();
    int status;

    if (child < 0) {
        perror("fork");
        return 1;
    }
    if (child == 0) {
        execlp("echo", "echo", "Child replaced by echo", (char *)NULL);
        perror("execlp");
        _exit(127);
    }
    if (waitpid(child, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }
    if (WIFEXITED(status))
        printf("Child exit status: %d\n", WEXITSTATUS(status));
    else
        printf("Child ended abnormally\n");
    return 0;
}
