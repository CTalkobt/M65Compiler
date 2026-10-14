/* test_bss_fill_xemu.c — crt0 fills BSS at startup
 *
 * Built twice by test_xemu.sh: by default BSS is zero (as C requires for
 * objects without an initializer); with -fbss-fill=$FF it is $FF. xemu's
 * RAM is zero at boot, so only the $FF build shows that the fill runs.
 *
 * Results at $C0C0: [0] g_byte, [1] the low byte of g_int, [2] g_arr[0],
 * [3] g_arr[299], [4] 01 if every byte of g_int and g_arr equals [0],
 * [5] 01: an uninitialized static local is in BSS too (filled like [0]),
 * [6] $AA
 */
#define OUT ((volatile unsigned char *)0xC0C0)

unsigned char g_byte;
unsigned int g_int;
unsigned char g_arr[300];

static unsigned char same(const unsigned char *p, unsigned int n, unsigned char v) {
    unsigned int i;
    for (i = 0; i < n; i++) if (p[i] != v) return 0;
    return 1;
}

int main(void) {
    static unsigned long s_local;
    unsigned char v = g_byte;
    OUT[0] = g_byte;
    OUT[1] = (unsigned char)g_int;
    OUT[2] = g_arr[0];
    OUT[3] = g_arr[299];
    OUT[4] = same((const unsigned char *)&g_int, 2, v) && same(g_arr, 300, v);
    OUT[5] = same((const unsigned char *)&s_local, 4, v);
    OUT[6] = 0xAA;
    return 0;
}
