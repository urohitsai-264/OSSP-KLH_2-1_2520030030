#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "trace_manager.h"

static int command_exists(const char *name) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "command -v %s >/dev/null 2>&1", name);
    return system(cmd) == 0;
}

int cmd_trace(int argc, char **argv) {
    char trace_output[256] = "/tmp/linux_os_trace.txt";
    char command[512] = {0};
    size_t index = 0;
    FILE *fp;
    char line[512];
    int i;

    if (argc < 2) {
        fprintf(stderr, "Usage: trace <command> [args...]\n");
        return -1;
    }

    if (!command_exists("strace")) {
        printf("strace is not available on this system.\n");
        printf("Install it with: sudo apt install strace\n");
        return 0;
    }

    for (i = 1; i < argc; i++) {
        if (index + strlen(argv[i]) + 2 >= sizeof(command)) {
            break;
        }
        if (i > 1) {
            command[index++] = ' ';
        }
        snprintf(command + index, sizeof(command) - index, "%s", argv[i]);
        index += strlen(argv[i]);
    }

    char exec_string[2048];
    snprintf(exec_string, sizeof(exec_string), "strace -qq -s 128 -o %s ", trace_output);
    strncat(exec_string, command, sizeof(exec_string) - strlen(exec_string) - 1);
    if (system(exec_string) != 0) {
        fprintf(stderr, "ERROR: strace failed for command '%s'.\n", command);
        return -1;
    }

    fp = fopen(trace_output, "r");
    if (fp == NULL) {
        perror("fopen trace output");
        return -1;
    }

    printf("TRACE SUMMARY\n");
    printf("Command traced: %s\n", command);
    printf("Actual strace output (first lines):\n");
    for (i = 0; i < 15 && fgets(line, sizeof(line), fp) != NULL; i++) {
        printf("%s", line);
    }
    fclose(fp);
    return 0;
}
