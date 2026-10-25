#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "system_info.h"

static int read_uptime_seconds(double *uptime_seconds) {
    FILE *fp = fopen("/proc/uptime", "r");
    if (fp == NULL) {
        perror("fopen /proc/uptime");
        return -1;
    }

    double value = 0.0;
    if (fscanf(fp, "%lf", &value) != 1) {
        fprintf(stderr, "Failed to parse /proc/uptime\n");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    *uptime_seconds = value;
    return 0;
}

void print_kernel_summary(void) {
    struct utsname info;
    char hostname[256] = {0};
    char os_name[256] = "unavailable";
    double uptime = 0.0;
    long cpu_cores = sysconf(_SC_NPROCESSORS_ONLN);

    if (uname(&info) != 0) {
        perror("uname");
        printf("Kernel Name     : unavailable\n");
        return;
    }

    if (gethostname(hostname, sizeof(hostname) - 1) == 0) {
        hostname[sizeof(hostname) - 1] = '\0';
    }

    FILE *os_file = fopen("/etc/os-release", "r");
    if (os_file != NULL) {
        char line[512];
        while (fgets(line, sizeof(line), os_file) != NULL) {
            if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
                char *value = line + 12;
                size_t len = strlen(value);
                while (len > 0 && (value[len - 1] == '\n' || value[len - 1] == '\r')) {
                    value[--len] = '\0';
                }
                if (len >= 2 && value[0] == '"' && value[len - 1] == '"') {
                    value[len - 1] = '\0';
                    value++;
                }
                snprintf(os_name, sizeof(os_name), "%s", value);
                break;
            }
        }
        fclose(os_file);
    }

    if (read_uptime_seconds(&uptime) == 0) {
        unsigned long long total_seconds = (unsigned long long)uptime;
        unsigned long long days = total_seconds / 86400ULL;
        unsigned long long hours = (total_seconds % 86400ULL) / 3600ULL;
        unsigned long long minutes = (total_seconds % 3600ULL) / 60ULL;
        unsigned long long seconds = total_seconds % 60ULL;
        printf("System Uptime   : %llu days, %02llu:%02llu:%02llu\n",
               days, hours, minutes, seconds);
    } else {
        printf("System Uptime   : unavailable\n");
    }

    printf("Kernel Name     : %s\n", info.sysname);
    printf("Kernel Release  : %s\n", info.release);
    printf("Kernel Version  : %s\n", info.version);
    printf("Machine         : %s\n", info.machine);
    printf("Hostname        : %s\n", hostname[0] != '\0' ? hostname : "unavailable");
    printf("Operating System: %s\n", os_name);
    printf("CPU Cores       : %ld\n", cpu_cores > 0 ? cpu_cores : 0);
}

void print_system_info(void) {
    print_kernel_summary();
}
