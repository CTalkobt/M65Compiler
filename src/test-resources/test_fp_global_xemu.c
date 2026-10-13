/* test_fp_global_xemu.c — a C global named fp (assembly _fp) is not
 * mistaken for the compiler's per-function frame-pointer variable.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] read of the global fp      [1] function pointer stored in fp
 *   [2] fp in a frame function      [3] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static int seven(void) { return 7; }
int (*fp)(void) = seven;

#pragma cc45 recurse
int depth(int n) { return n ? depth(n - 1) + fp() : 0; }

int main(void) {
    OUT[0] = fp == seven;
    OUT[1] = fp() == 7;
    OUT[2] = depth(3) == 21;
    OUT[3] = 0xAA;
    return 0;
}
