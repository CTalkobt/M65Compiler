/* printf return path test */

#include <stdio.h>

int main() {
    asm("tsx");
    asm("stx $0800");   /* SP at start */

    printf("x");        /* simplest printf - single literal char, no %d */

    asm("tsx");
    asm("stx $0801");   /* SP after printf literal */

    printf("%d", 7);    /* printf with %d */

    asm("tsx");
    asm("stx $0802");   /* SP after printf %d */

    return 0;
}
