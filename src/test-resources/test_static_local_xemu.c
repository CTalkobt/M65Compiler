/* test_static_local_xemu.c — static locals: initialized ones in DATA,
 * uninitialized ones in BSS (zeroed by crt0, like uninitialized globals);
 * an explicit = 0 stays in DATA
 *
 * Results at $C0C0 (01 = pass):
 *   [0] first call: initial values (5, 0, "hi", 0, a zero array, struct,
 *       pointer and float)
 *   [1] second call: the values kept from the first
 *   [2] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
struct P { int x, y; };
int f(void) { static int a = 5; static int b; static char s[] = "hi"; static int z = 0; static int arr[4]; static struct P p; static int *ptr; static float fl;
  b++; arr[3] += 2; p.y += 3; return a++ + b + s[0] + z + arr[3] + p.y + (ptr == 0) + (fl == 0.0); }
int main(void) { int r1 = f(), r2 = f(); OUT[0] = r1 == 5 + 1 + 'h' + 0 + 2 + 3 + 1 + 1; OUT[1] = r2 == 6 + 2 + 'h' + 4 + 6 + 1 + 1; OUT[2] = 0xAA; return 0; }
