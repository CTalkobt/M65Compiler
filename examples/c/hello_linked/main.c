/* hello_linked — Minimal linked C program for MEGA65
 *
 * Demonstrates the relocatable compilation workflow:
 *   cc45 -c main.c -o main.o45      (compile to object)
 *   ln45 -basic -o hello.prg crt0.o45 main.o45 c45.lib  (link)
 *
 * The linker resolves printf/puts from c45.lib and prepends
 * the CRT startup code from crt0.o45.
 */

#include <stdio.h>
#include <string.h>

int main() {
    char buf[32];

    puts("linked hello!");

    strcpy(buf, "built with cc45");
    puts(buf);

    printf("string length: %d\n", strlen(buf));

    return 0;
}
