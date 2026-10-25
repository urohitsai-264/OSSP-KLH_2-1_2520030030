#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#define CMD_MAX_ARGS 32

int parse_command(const char *input, char *argv[CMD_MAX_ARGS], int *argc);

#endif
