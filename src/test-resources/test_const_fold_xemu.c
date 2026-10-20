/* test_const_fold_xemu.c — the constant folder decides on a literal's value
 * as its type says, and _Alignas takes constant expressions (Issue #93)
 *
 * (unsigned)65536 is 0 in a 16-bit unsigned: the folder took the raw 65536
 * as true, kept the wrong branch of if / ?: and ran the while loop.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] if ((unsigned)65536) takes the else branch
 *   [1] (unsigned)65536 ? 5 : 6 is 6; (unsigned char)256 ? 5 : 6 is 6
 *   [2] while ((unsigned)65536) does not run; for (; (unsigned)65536; ) neither
 *   [3] _Bool b = (unsigned)65536 is 0
 *   [4] _Alignas(sizeof x * 2): both arrays on a 4-byte boundary (was 1)
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

int x;
char pad0;    /* (1-byte neighbours: without the alignment the arrays land off a boundary) */
_Alignas(sizeof x * 2) char al4[3];
char pad1;
_Alignas(sizeof x * 2) char al4b[3];
char pad2;

int main(void) {
    unsigned char r = 0, w = 0;
    _Bool b = (unsigned)65536;
    if ((unsigned)65536) r = 1; else r = 2;
    OUT[0] = r == 2;
    OUT[1] = ((unsigned)65536 ? 5 : 6) == 6 && ((unsigned char)256 ? 5 : 6) == 6;
    while ((unsigned)65536) { w = 1; break; }
    for (; (unsigned)65536; ) { w = 2; break; }
    OUT[2] = w == 0;
    OUT[3] = b == 0;
    OUT[4] = ((unsigned)al4 & 3) == 0 && ((unsigned)al4b & 3) == 0;
    OUT[5] = 0xAA;
    return 0;
}
