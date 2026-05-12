#include <stdio.h>
#include <stdlib.h>

int main() {
    char buf[8];

    itoa(42, buf, 10);
    puts(buf);

    printf("%d\n", 7);

    puts("done");

    asm("bra *");
    return 0;
}
