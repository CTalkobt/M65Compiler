/* hello_linked — Minimal linked C program for MEGA65
 *
 * Demonstrates the relocatable compilation workflow:
 *   cc45 -c main.c -o main.o45      (compile to object)
 *   ln45 -basic -o hello.prg main.o45 c45.lib  (link)
 *
 * The linker resolves puts/strlen from c45.lib and prepends
 * the CRT startup code.
 */

#include <stdio.h>
#include <string.h>

int main() {
    int len;

    puts("linked hello!");

    len = strlen("built with cc45");

    if (len == 15)
        puts("strlen: pass");
    else
        puts("strlen: fail");

    return 0;
}
