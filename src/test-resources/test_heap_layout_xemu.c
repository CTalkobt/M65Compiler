/* test_heap_layout_xemu.c — the heap of a program that ends above $B000
 *
 * malloc's heap runs from the end of the program to __stack_size bytes below
 * the CPU stack top ($BFFF): 4096 by default, so a program ending at $B000
 * has no heap (ln45 warns). #pragma cc45 stack_size(1024) leaves the stack
 * 1KB and gives the heap $B0xx-$BBFF.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] malloc succeeds, after the program (above big[])
 *   [1] the blocks end below the stack's share ($BC00)
 *   [2] free and malloc again reuse the space; deep recursion still works
 *   [3] $AA sentinel
 */
#pragma cc45 stack_size(1024)
#include <stdlib.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static char big[0x8800];   /* (the program ends at about $B000) */

#pragma cc45 recurse
static int depth(int n) { volatile char pad[16]; pad[0] = (char)n; return n ? depth(n - 1) + pad[0] - n + 1 : 0; }

int main(void) {
    char *a = malloc(1000), *b = malloc(1000), *c;
    big[sizeof big - 1] = 1;
    OUT[0] = a && b && (unsigned)a > (unsigned)&big[sizeof big - 1];
    OUT[1] = a && b && (unsigned)b + 1000 <= 0xBC00u && (unsigned)a + 1000 <= 0xBC00u;
    free(a);
    c = malloc(900);
    OUT[2] = c == a && depth(20) == 20;
    OUT[3] = 0xAA;
    return 0;
}
