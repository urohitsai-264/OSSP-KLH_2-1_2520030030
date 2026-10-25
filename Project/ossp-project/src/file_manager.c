#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "file_manager.h"

int cmd_file_demo(void) {
    const char *path = "./linux_os_file_demo.txt";
    const char *message = "Linux file I/O demonstration";
    char buffer[256] = {0};
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    ssize_t written;
    off_t seek_pos;
    ssize_t bytes_read;

    if (fd < 0) {
        perror("open");
        return -1;
    }

    written = write(fd, message, strlen(message));
    if (written < 0) {
        perror("write");
        close(fd);
        return -1;
    }

    seek_pos = lseek(fd, 0, SEEK_SET);
    if (seek_pos < 0) {
        perror("lseek");
        close(fd);
        return -1;
    }

    bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("read");
        close(fd);
        return -1;
    }
    buffer[bytes_read] = '\0';

    printf("FILE OPERATIONS\n");
    printf("Path          : %s\n", path);
    printf("File descriptor: %d\n", fd);
    printf("Bytes written : %ld\n", (long)written);
    printf("Bytes read    : %ld\n", (long)bytes_read);
    printf("Seek position : %ld\n", (long)seek_pos);
    printf("Read back     : %s\n", buffer);

    if (close(fd) == 0) {
        printf("Cleanup       : file closed successfully.\n");
    } else {
        perror("close");
    }

    if (unlink(path) != 0 && errno != ENOENT) {
        perror("unlink");
    }

    return 0;
}

int cmd_inode_demo(void) {
    const char *path = "./linux_os_inode_demo.txt";
    struct stat st;
    int fd = open(path, O_CREAT | O_RDWR, 0644);

    if (fd < 0) {
        perror("open");
        return -1;
    }

    if (write(fd, "inode demo", 11) < 0) {
        perror("write");
        close(fd);
        return -1;
    }
    close(fd);

    if (stat(path, &st) != 0) {
        perror("stat");
        unlink(path);
        return -1;
    }

    printf("INODE INFORMATION\n");
    printf("Path          : %s\n", path);
    printf("Inode         : %ld\n", (long)st.st_ino);
    printf("Mode          : %o\n", st.st_mode & 07777);
    printf("Size          : %ld bytes\n", (long)st.st_size);
    printf("Link count    : %ld\n", (long)st.st_nlink);
    printf("UID/GID       : %ld/%ld\n", (long)st.st_uid, (long)st.st_gid);

    if (unlink(path) != 0 && errno != ENOENT) {
        perror("unlink");
    }
    return 0;
}

int cmd_mmap_demo(void) {
    const char *path = "./linux_os_mmap_demo.txt";
    const char *message = "memory mapped file content";
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    char *mapped;
    struct stat st;
    size_t length;

    if (fd < 0) {
        perror("open");
        return -1;
    }

    if (write(fd, message, strlen(message)) < 0) {
        perror("write");
        close(fd);
        return -1;
    }

    if (fstat(fd, &st) != 0) {
        perror("fstat");
        close(fd);
        return -1;
    }

    length = (size_t)st.st_size;
    mapped = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }

    printf("MMAP DEMONSTRATION\n");
    printf("File          : %s\n", path);
    printf("Mapping size  : %zu bytes\n", length);
    printf("Mapped content: %.*s\n", (int)length, mapped);
    printf("Cleanup       : munmap() and file close.\n");

    if (munmap(mapped, length) != 0) {
        perror("munmap");
    }
    close(fd);
    if (unlink(path) != 0 && errno != ENOENT) {
        perror("unlink");
    }
    return 0;
}
