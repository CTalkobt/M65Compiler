/* hello_linked — Minimal linked C program for MEGA65 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char buf[8];

    puts("linked hello!");

    itoa(42, buf, 10);
    puts(buf);

    printf("string length: %d\n", strlen("hello"));

    puts("done!");

    return 0;
}
