#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ipc_manager.h"

int cmd_pipe_demo(void) {
    int fd[2];
    pid_t child;
    char message[] = "Linux pipes are real OS IPC channels.";
    char buffer[256];
    ssize_t bytes_read;
    int status;

    if (pipe(fd) == -1) {
        perror("pipe");
        return -1;
    }

    child = fork();
    if (child < 0) {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return -1;
    }

    if (child == 0) {
        close(fd[0]);
        if (write(fd[1], message, strlen(message)) == -1) {
            perror("write");
            close(fd[1]);
            _exit(1);
        }
        close(fd[1]);
        _exit(0);
    }

    close(fd[1]);
    bytes_read = read(fd[0], buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("read");
        close(fd[0]);
        return -1;
    }
    buffer[bytes_read] = '\0';
    close(fd[0]);

    printf("PIPE DEMONSTRATION\n");
    printf("Parent PID : %ld\n", (long)getpid());
    printf("Child PID  : %ld\n", (long)child);
    printf("Message sent    : %s\n", message);
    printf("Message received: %s\n", buffer);

    if (waitpid(child, &status, 0) == -1) {
        perror("waitpid");
        return -1;
    }

    if (WIFEXITED(status)) {
        printf("Child exit status: %d\n", WEXITSTATUS(status));
    }
    return 0;
}

int cmd_fifo_demo(void) {
    const char *fifo_path = "/tmp/linux_os_fifo_demo";
    char message[] = "Named pipe test from Linux console";
    char buffer[256];
    pid_t child;
    int status;
    int fd;
    ssize_t bytes_read;

    if (mkfifo(fifo_path, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        return -1;
    }

    child = fork();
    if (child < 0) {
        perror("fork");
        unlink(fifo_path);
        return -1;
    }

    if (child == 0) {
        fd = open(fifo_path, O_WRONLY);
        if (fd < 0) {
            perror("open fifo for write");
            _exit(1);
        }
        if (write(fd, message, strlen(message)) == -1) {
            perror("write");
            close(fd);
            _exit(1);
        }
        close(fd);
        _exit(0);
    }

    fd = open(fifo_path, O_RDONLY);
    if (fd < 0) {
        perror("open fifo for read");
        unlink(fifo_path);
        return -1;
    }

    bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("read");
        close(fd);
        unlink(fifo_path);
        return -1;
    }
    buffer[bytes_read] = '\0';
    close(fd);

    printf("FIFO DEMONSTRATION\n");
    printf("FIFO path : %s\n", fifo_path);
    printf("Writer    : child process %ld\n", (long)child);
    printf("Reader    : parent process %ld\n", (long)getpid());
    printf("Message   : %s\n", buffer);

    if (waitpid(child, &status, 0) == -1) {
        perror("waitpid");
        unlink(fifo_path);
        return -1;
    }

    if (unlink(fifo_path) == 0) {
        printf("Cleanup   : FIFO removed successfully.\n");
    } else {
        perror("unlink");
        printf("Cleanup   : failed to remove FIFO.\n");
    }

    return 0;
}
