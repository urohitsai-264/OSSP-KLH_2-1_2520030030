#include <stdio.h>
#include <stdlib.h>

#include "shell.h"

int main(void) {
    print_banner();
    install_shell_signal_handlers();
    shell_run();
    return 0;
}
