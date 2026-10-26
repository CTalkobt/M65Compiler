/* test_bool_xemu.c — conversions to _Bool (C11 6.3.1.2: 0 or 1) and its
 * size in memory (1 byte)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a global assigned a constant above 255   [1] ... an int variable
 *   [2] a local initialized from an int          [3] a return value
 *   [4] an argument                              [5] from a long
 *   [6] from a pointer                           [7] a cast (_Bool)
 *   [8] from a float
 *   [9] stores are 1 byte: the next global, an array element, a struct
 *       member (a 16-bit store wrote the byte after it too)
 *   [10] compound assignment (t += 2 is 1)       [11] !
 */
#define OUT ((volatile unsigned char *)0xC0C0)
_Bool g, h;
unsigned char after_h = 0x77;
_Bool arr[4];
struct S { _Bool a; unsigned char b; } s = { 0, 0x55 };
_Bool f(int x) { _Bool b = x; return b; }
_Bool r(int x) { return x; }
void p(_Bool v) { OUT[4] = v; }
int main(void) {
    int big = 512; long l = 0x10000L; unsigned char *ptr = (unsigned char *)0x100;
    g = 512;
    h = big;
    OUT[0] = g; OUT[1] = h; OUT[2] = f(512); OUT[3] = r(256);
    p(big);
    { _Bool lb = l; OUT[5] = lb; }
    { _Bool pb = ptr; OUT[6] = pb; }
    { _Bool cb = (_Bool)big; OUT[7] = cb; }
    { _Bool fb = 0.5f; OUT[8] = fb; }
    arr[1] = 300; s.a = 2;
    OUT[9] = after_h == 0x77 && arr[1] == 1 && arr[2] == 0 && s.a == 1 && s.b == 0x55;
    { _Bool t = 0; t += 2; OUT[10] = t; t = !t; OUT[11] = !t; }
    return 0;
}
