#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

int cmd_memory_info(void);
int cmd_virtual_memory(void);
int cmd_malloc_demo(void);
int cmd_cow_demo(void);
int cmd_memcheck(void);
int cmd_leakcheck(int argc, char **argv);

#endif
