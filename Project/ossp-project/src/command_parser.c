#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_parser.h"

static void trim_spaces(char *str) {
    if (str == NULL) {
        return;
    }

    char *start = str;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}

int parse_command(const char *input, char *argv[CMD_MAX_ARGS], int *argc) {
    char *buffer;
    char *token;
    size_t input_len;
    int count = 0;

    if (input == NULL || argv == NULL || argc == NULL) {
        return -1;
    }

    for (int i = 0; i < CMD_MAX_ARGS; i++) {
        argv[i] = NULL;
    }

    input_len = strlen(input);
    buffer = malloc(input_len + 1);
    if (buffer == NULL) {
        return -1;
    }
    memcpy(buffer, input, input_len + 1);

    trim_spaces(buffer);
    if (buffer[0] == '\0') {
        free(buffer);
        *argc = 0;
        return 0;
    }

    token = strtok(buffer, " \t\n\r");
    while (token != NULL && count < CMD_MAX_ARGS - 1) {
        argv[count] = strdup(token);
        if (argv[count] == NULL) {
            for (int i = 0; i < count; i++) {
                free(argv[i]);
            }
            free(buffer);
            return -1;
        }
        count++;
        token = strtok(NULL, " \t\n\r");
    }

    argv[count] = NULL;
    *argc = count;
    free(buffer);
    return 0;
}
