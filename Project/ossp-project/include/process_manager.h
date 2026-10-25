#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>

int cmd_execute(int argc, char **argv);
int cmd_create_process(void);
int cmd_status(int argc, char **argv);
int cmd_tree(void);
int cmd_zombie_demo(void);
int cmd_reap_zombie(void);
int cmd_orphan_demo(void);
int cmd_signal_demo(int argc, char **argv);

#endif
