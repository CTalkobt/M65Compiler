/* test_basic_fin_xemu.c — basic45 --compile's number reader (#369):
 * basic_fin.s45's __bas_fin against the C library's strtof, whose algorithm
 * it follows step for step: the same 5 bytes and the same end for each text
 * (signs, spaces, fractions, both E's, exponent signs, more than 10 digits,
 * overflow and underflow, no number, a number followed by more).
 *
 * Results at $C0C0: [0] the count that differed (0 = pass), [1] the first
 * one's index + 1 (0: none), [2] $AA
 */
#include <stdlib.h>
#include <string.h>

extern unsigned char _fin_buf[82];
extern unsigned char _fin_i;
extern unsigned char _float_a[5];
extern void _bas_fin(void);

#define OUT ((volatile unsigned char *)0xC0C0)

static const char *const texts[] = {
    "0", "1", "123", "-45.5", "  +7", ".5", "5.", ".", "1e3", "1E3", "1e", "1e+", "2.5e-3", "-2.5E+3",
    "123456789012345", "0.000000000123456789", "1e38", "1e39", "1e-39", "1e-40", "9999999999", "4294967296",
    "3.14159265358979", "-0", "abc", "", "12abc", "1.2.3", "  ", "1e200", "1e-200", "000123", ".000001",
    "99999999999999999999e-10", "0.1", "0.2", "0.3", "1.7014118346e38", "2.9387358771e-39", "65535", "-32768",
    "12345.678", "1e05", "7e-0005", "+", "-", "-.", "\t12", "\n3", "8.25e1x",
};

int main(void) {
    unsigned char k, bad = 0, first = 0;
    for (k = 0; k < sizeof texts / sizeof texts[0]; k++) {
        const char *t = texts[k];
        char *end;
        float f = strtof((char *)t, &end);
        unsigned char n = (unsigned char)strlen(t);
        memcpy(_fin_buf, t, n + 1);
        _bas_fin();
        if (memcmp(_float_a, &f, 5) || _fin_i != (unsigned char)(end - t)) {
            bad++;
            if (!first) first = k + 1;
        }
    }
    OUT[0] = bad;
    OUT[1] = first;
    OUT[2] = 0xAA;
    return 0;
}
