CC = gcc
CFLAGS = -Wall -Wextra -g -pthread -std=c11
CPPFLAGS = -Iinclude
TARGET = ossp_monitor

SRCS = src/main.c src/command_parser.c src/system_info.c src/shell.c src/process_manager.c src/ipc_manager.c src/memory_manager.c src/file_manager.c src/thread_manager.c src/trace_manager.c src/monitor.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
