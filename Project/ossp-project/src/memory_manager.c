#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "memory_manager.h"

static int check_tool(const char *name) {
    char command[256];
    snprintf(command, sizeof(command), "command -v %s >/dev/null 2>&1", name);
    return system(command) == 0;
}

static int run_system_command(const char *command) {
    return system(command);
}

static int read_meminfo_value(const char *key, long *value) {
    FILE *fp = fopen("/proc/meminfo", "r");
    char line[256];

    if (fp == NULL) {
        perror("fopen /proc/meminfo");
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, key, strlen(key)) == 0) {
            char *pos = strchr(line, ':');
            if (pos == NULL) {
                fclose(fp);
                return -1;
            }
            pos++;
            while (*pos == ' ' || *pos == '\t') {
                pos++;
            }
            errno = 0;
            *value = strtol(pos, NULL, 10);
            if (errno != 0) {
                fclose(fp);
                return -1;
            }
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);
    return -1;
}

int cmd_memory_info(void) {
    long total_kb = 0;
    long available_kb = 0;
    long free_kb = 0;
    long used_kb = 0;

    if (read_meminfo_value("MemTotal:", &total_kb) != 0 ||
        read_meminfo_value("MemAvailable:", &available_kb) != 0 ||
        read_meminfo_value("MemFree:", &free_kb) != 0) {
        fprintf(stderr, "ERROR: unable to read /proc/meminfo fully.\n");
        return -1;
    }

    used_kb = total_kb - available_kb;
    printf("MEMORY INFORMATION\n");
    printf("Total RAM       : %ld kB\n", total_kb);
    printf("Available RAM   : %ld kB\n", available_kb);
    printf("Free RAM        : %ld kB\n", free_kb);
    printf("Used RAM        : %ld kB\n", used_kb);
    printf("RAM usage       : %.2f%%\n", ((double)used_kb / (double)total_kb) * 100.0);
    return 0;
}

int cmd_virtual_memory(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    FILE *fp = fopen("/proc/self/maps", "r");
    char line[512];
    int count = 0;

    printf("VIRTUAL ADDRESS SPACE\n");
    printf("Page Size       : %ld bytes\n", page_size);

    if (fp == NULL) {
        perror("fopen /proc/self/maps");
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL && count < 12) {
        printf("%s", line);
        count++;
    }
    fclose(fp);

    printf("Detected through the process virtual address map.\n");
    printf("Executable mappings, heap, stack, and shared objects are visible above.\n");
    return 0;
}

int cmd_malloc_demo(void) {
    int *numbers = NULL;
    int *grown = NULL;
    int *zeroed = NULL;

    numbers = malloc(5 * sizeof(int));
    if (numbers == NULL) {
        fprintf(stderr, "ERROR: malloc failed.\n");
        return -1;
    }
    for (int i = 0; i < 5; i++) {
        numbers[i] = i + 1;
    }

    zeroed = calloc(5, sizeof(int));
    if (zeroed == NULL) {
        fprintf(stderr, "ERROR: calloc failed.\n");
        free(numbers);
        return -1;
    }

    grown = realloc(numbers, 10 * sizeof(int));
    if (grown == NULL) {
        fprintf(stderr, "ERROR: realloc failed.\n");
        free(numbers);
        free(zeroed);
        return -1;
    }

    printf("MALLOC / CALLOC / REALLOC / FREE\n");
    printf("malloc() allocated 5 integers.\n");
    printf("calloc() allocated 5 zero-initialized integers.\n");
    printf("realloc() expanded memory to 10 integers.\n");
    printf("First value after realloc: %d\n", grown[0]);
    printf("Memory cleanup: free() called for both allocations.\n");

    free(grown);
    free(zeroed);
    return 0;
}

int cmd_cow_demo(void) {
    int *shared = calloc(4096, sizeof(char));
    pid_t child;
    int status;

    if (shared == NULL) {
        perror("calloc");
        return -1;
    }
    shared[0] = 10;

    child = fork();
    if (child < 0) {
        perror("fork");
        free(shared);
        return -1;
    }

    if (child == 0) {
        printf("[child] initial value before any write: %d\n", shared[0]);
        shared[0] = 20;
        printf("[child] modified value after fork: %d\n", shared[0]);
        free(shared);
        _exit(0);
    }

    sleep(1);
    printf("[parent] value visible before parent write: %d\n", shared[0]);
    shared[0] = 30;
    printf("[parent] modified value after parent write: %d\n", shared[0]);

    waitpid(child, &status, 0);
    free(shared);
    printf("Copy-on-Write: both parent and child initially saw the same memory, but writes diverged after fork.\n");
    printf("This is observed through fork semantics; Linux implements COW by sharing pages until one side writes.\n");
    return 0;
}

static int run_valgrind_demo(const char *demo_name, const char *fixed_flag) {
    char compile_cmd[256];
    char run_cmd[512];
    char output_path[256];
    FILE *fp;
    char line[512];

    if (!check_tool("valgrind")) {
        printf("Valgrind was not found on this system.\n");
        printf("Please install Valgrind before running leakcheck.\n");
        return 0;
    }

    snprintf(compile_cmd, sizeof(compile_cmd), "gcc -g -O0 demos/%s -o /tmp/%s 2>/dev/null", demo_name, demo_name);
    if (run_system_command(compile_cmd) != 0) {
        fprintf(stderr, "ERROR: failed to compile the Valgrind demo program.\n");
        return -1;
    }

    snprintf(output_path, sizeof(output_path), "/tmp/%s.out", demo_name);
    if (fixed_flag != NULL) {
        snprintf(run_cmd, sizeof(run_cmd), "valgrind --leak-check=full --show-leak-kinds=all /tmp/%s > %s 2>&1", demo_name, output_path);
    } else {
        snprintf(run_cmd, sizeof(run_cmd), "valgrind --leak-check=full --show-leak-kinds=all /tmp/%s > %s 2>&1", demo_name, output_path);
    }

    if (run_system_command(run_cmd) != 0) {
        fprintf(stderr, "ERROR: Valgrind returned a non-zero status.\n");
        return -1;
    }

    fp = fopen(output_path, "r");
    if (fp == NULL) {
        perror("fopen Valgrind output");
        return -1;
    }

    printf("LEAKCHECK\n");
    printf("Running Valgrind Memcheck...\n\n");
    printf("Actual Valgrind analysis:\n");
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "definitely lost") != NULL ||
            strstr(line, "indirectly lost") != NULL ||
            strstr(line, "possibly lost") != NULL ||
            strstr(line, "still reachable") != NULL ||
            strstr(line, "ERROR SUMMARY") != NULL) {
            printf("%s", line);
        }
    }
    fclose(fp);
    return 0;
}

int cmd_memcheck(void) {
    if (!check_tool("valgrind")) {
        printf("Valgrind was not found on this system.\n");
        printf("Please install Valgrind before running memcheck.\n");
        return 0;
    }

    printf("MEMCHECK\n");
    printf("This demonstration runs a controlled invalid memory access example in an isolated program.\n");
    if (run_system_command("gcc -g -O0 demos/memcheck_demo.c -o /tmp/memcheck_demo >/dev/null 2>&1") != 0) {
        fprintf(stderr, "ERROR: failed to compile memcheck demo.\n");
        return -1;
    }
    return run_system_command("valgrind --track-origins=yes --error-exitcode=1 /tmp/memcheck_demo >/tmp/memcheck_output.txt 2>&1") == 0 ? 0 : 0;
}

int cmd_leakcheck(int argc, char **argv) {
    const char *target = "leak_demo.c";

    if (argc > 1 && strcmp(argv[1], "--fixed") == 0) {
        target = "leak_demo_fixed.c";
    }

    if (strcmp(target, "leak_demo_fixed.c") == 0) {
        printf("Running the leak-free version under Valgrind.\n");
    }

    return run_valgrind_demo(target, NULL);
}
