#ifndef SHELL_H
#define SHELL_H

void print_banner(void);
int shell_run(void);
void install_shell_signal_handlers(void);
int shell_exit_requested(void);
void clear_shell_exit_flag(void);

#endif
