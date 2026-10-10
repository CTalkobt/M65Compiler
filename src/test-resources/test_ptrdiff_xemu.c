/* test_ptrdiff_xemu.c — pointer differences count elements (signed)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] K&R parameters: b - a                 [1] prototype parameters: b - a
 *   [2] negative difference (short *)          [3] struct of 3 bytes via pointer vars
 *   [4] &s[7] - &s[2]                          [5] (p - q) < 0 is signed
 *   [6] long elements                          [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
struct S3 { char a, b, c; };
struct S3 s[10];
unsigned short arr[8];
long la[6];
int d1(a, b) unsigned short *a, *b; { return b - a; }
int d2(unsigned short *a, unsigned short *b) { return b - a; }
int main(void) {
    unsigned short *p = arr + 1, *q = arr + 6;
    struct S3 *x = &s[7], *y = &s[2];
    signed int d = p - q;
    OUT[0] = d1(arr, arr + 3) == 3;
    OUT[1] = d2(arr, arr + 3) == 3;
    OUT[2] = d == -5;
    OUT[3] = (x - y) == 5;
    OUT[4] = (&s[7] - &s[2]) == 5;
    OUT[5] = (p - q) < 0;
    OUT[6] = (&la[5] - &la[1]) == 4;
    OUT[7] = 0xAA;
    return 0;
}
