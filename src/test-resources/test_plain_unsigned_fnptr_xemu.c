/* test_plain_unsigned_fnptr_xemu.c — 'signed' / 'unsigned' alone as the
 * return type of a function pointer at file scope, and sizeof of a function
 * pointer type (both were parse errors), and a file-scope function
 * pointer's 'signed' result (it was taken as unsigned)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] unsigned (*fp)(long): the result is unsigned
 *   [1] signed (*sp)(long): the result is signed
 *   [2] const unsigned (*cp)(long), static unsigned (*tbl[2])(long)
 *   [3] sizeof(unsigned (*)(long)) == sizeof(int (*)(long)) == 2
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

unsigned int ret_max(long v) { return (unsigned int)(0xFFFF - (unsigned int)v); }
signed int ret_neg(long v) { return (signed int)(-(signed int)v); }

unsigned (*fp)(long) = ret_max;
signed (*sp)(long) = ret_neg;
const unsigned (*cp)(long) = ret_max;
static unsigned (*tbl[2])(long) = { ret_max, ret_max };
unsigned szu = sizeof(unsigned (*)(long));
unsigned szi = sizeof(int (*)(long));

int main(void) {
    OUT[0] = fp(0) > 0 && fp(1) == 0xFFFE;
    OUT[1] = sp(1) < 0 && sp(2) == -2;
    OUT[2] = cp(2) == 0xFFFD && tbl[1](3) == 0xFFFC && tbl[0] == ret_max;
    OUT[3] = szu == 2 && szi == 2;
    OUT[4] = 0xAA;
    return 0;
}
