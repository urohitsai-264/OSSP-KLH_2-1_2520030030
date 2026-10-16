#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        return 1;
    }
    if (child == 0) {
        (void)dprintf(STDOUT_FILENO, "Child process: pid=%ld, parent=%ld\n",
                      (long)getpid(), (long)getppid());
        _exit(EXIT_SUCCESS);
    }

    printf("Parent process: pid=%ld, child=%ld\n", (long)getpid(), (long)child);
    if (waitpid(child, NULL, 0) < 0) {
        perror("waitpid");
        return 1;
    }
    return 0;
}
