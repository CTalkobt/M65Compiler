/* hello_linked — SP tracking test */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Store SP values in screen RAM where nothing will overwrite them */
char *screen = 0x0800;

int main() {
    char buf[8];

    /* Log SP low byte at each stage to screen RAM */
    asm("tsx");
    asm("stx $0800");  /* screen[0] = SPL at start */

    puts("linked hello!");
    asm("tsx");
    asm("stx $0801");  /* screen[1] = SPL after puts */

    itoa(42, buf, 10);
    puts(buf);
    asm("tsx");
    asm("stx $0802");  /* screen[2] = SPL after itoa+puts */

    printf("len: %d\n", strlen("hello"));
    asm("tsx");
    asm("stx $0803");  /* screen[3] = SPL after printf */

    puts("done!");
    asm("tsx");
    asm("stx $0804");  /* screen[4] = SPL after final puts */

    return 0;
}
