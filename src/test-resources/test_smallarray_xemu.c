/* test_smallarray_xemu.c — elements of global arrays of at most 256 bytes
 * are addressed as name+offset,y (no pointer built in zero page); larger
 * arrays the general way
 *
 * Results at $C0C0 (01 = pass):
 *   [0] unsigned char a[256] (the largest small array): every element
 *       written and read through a char and through an int index
 *   [1] int b[128] and a pointer array: 16-bit elements, the last at 254
 *   [2] arrays of 2- and 4-byte structs (a member at an offset), signed
 *       char indices, a[i] = a[j] copies, constants and computed values
 *   [3] unsigned char c[257] and int d[200] (over 256 bytes: the general
 *       code) next to the small ones
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

unsigned char a[256];
signed int b[128];
const char *p[8];
struct P2 { unsigned char x, y; } s2[100];
struct P4 { signed int v; unsigned char t, u; } s4[60];
unsigned char c[257];
signed int d[200];
static const char hello[] = "hello";

int main(void) {
    unsigned char i, ok = 1;
    unsigned int n;
    for (n = 0; n < 256; n++) a[n] = (unsigned char)(n ^ 0x5A);
    for (n = 0; n < 256; n++) if (a[n] != (unsigned char)(n ^ 0x5A)) ok = 0;
    i = 0;
    do { if (a[i] != (unsigned char)(i ^ 0x5A)) ok = 0; a[i] = i; i++; } while (i != 0);
    for (n = 0; n < 256; n++) if (a[n] != n) ok = 0;
    OUT[0] = ok;
    ok = 1;
    for (i = 0; i < 128; i++) b[i] = (signed int)i * -200;
    for (n = 0; n < 128; n++) if (b[n] != (signed int)n * -200) ok = 0;
    if (b[127] != -25400) ok = 0;
    for (i = 0; i < 8; i++) p[i] = hello + (i & 3);
    for (i = 0; i < 8; i++) if (*p[i] != hello[i & 3]) ok = 0;
    OUT[1] = ok;
    ok = 1;
    for (i = 0; i < 100; i++) { s2[i].x = i; s2[i].y = 200 - i; }
    for (i = 0; i < 60; i++) { s4[i].v = (signed int)i - 30; s4[i].t = i; s4[i].u = 7; }
    {
        signed char k;
        for (k = 0; k < 60; k++) if (s4[k].v != k - 30 || s4[k].u != 7 || s2[k].y != 200 - k) ok = 0;
    }
    for (i = 1; i < 100; i++) s2[i].x = s2[i - 1].x + s2[i].y;
    {
        unsigned char want = 0;
        for (i = 1; i < 100; i++) { want = want + (200 - i); if (s2[i].x != want) ok = 0; }
    }
    for (i = 0; i < 60; i++) s4[i].v = s4[59 - i].t;
    for (i = 0; i < 30; i++) if (s4[i].v != 59 - i) ok = 0;
    OUT[2] = ok;
    ok = 1;
    for (n = 0; n < 257; n++) c[n] = (unsigned char)n;
    for (n = 0; n < 200; n++) d[n] = (signed int)n - 100;
    if (c[256] != 0 || c[255] != 255 || d[199] != 99 || d[0] != -100) ok = 0;
    for (n = 0; n < 200; n++) if (d[n] != (signed int)n - 100 || c[n] != n) ok = 0;
    if (a[255] != 255 || b[0] != 0) ok = 0;
    OUT[3] = ok;
    OUT[4] = 0xAA;
    return 0;
}
