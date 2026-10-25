#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *ptr = malloc(sizeof(int) * 10);
    if (ptr == NULL) {
        return 1;
    }
    ptr[0] = 42;
    printf("Value in fixed block: %d\n", ptr[0]);
    free(ptr);
    return 0;
}
