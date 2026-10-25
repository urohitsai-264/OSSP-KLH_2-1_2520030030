#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *buf = malloc(4 * sizeof(int));
    if (buf == NULL) {
        return 1;
    }
    buf[0] = 10;
    buf[1] = 20;
    buf[2] = 30;
    buf[3] = 40;
    buf[10] = 99;
    free(buf);
    return 0;
}
