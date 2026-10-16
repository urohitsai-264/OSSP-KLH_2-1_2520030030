CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11
SOURCES = $(wildcard Skill-*.c) ShellForge.c
PROGRAMS = $(patsubst %.c,build/%,$(SOURCES))

.PHONY: all test clean

all: $(PROGRAMS)

build:
	mkdir -p build

build/%: %.c | build
	$(CC) $(CFLAGS) $< -o $@

test: all
	sh tests/run_tests.sh

clean:
	rm -rf build
