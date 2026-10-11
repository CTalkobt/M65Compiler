/* test_stdint_xemu.c — <stdint.h> exact-width types keep their signedness
 * whatever the default (plain int/long are unsigned in cc45)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] int8_t  -1 < 0
 *   [1] int16_t -1 < 0
 *   [2] int32_t -1 < 0
 *   [3] uint8_t/uint16_t/uint32_t all-ones are positive
 *   [4] sizes 1, 2, 4
 *   [5] int16_t sign-extends to int32_t; arithmetic right shift
 *   [6] intptr_t, int_fast16_t, int_least32_t are signed
 *   [7] $AA sentinel
 */
#include <stdint.h>
#define OUT ((volatile unsigned char *)0xC0C0)

int main(void) {
    int8_t a = -1;
    int16_t b = -1;
    int32_t c = -1;
    uint8_t d = 255;
    uint16_t e = 65535u;
    uint32_t f = 0xFFFFFFFFUL;
    int32_t w;

    OUT[0] = a < 0;
    OUT[1] = b < 0;
    OUT[2] = c < 0;
    OUT[3] = d > 0 && e > 0 && f > 0;
    OUT[4] = sizeof(int8_t) == 1 && sizeof(int16_t) == 2 && sizeof(int32_t) == 4;
    w = b;
    OUT[5] = w == -1L && (b >> 1) == -1;
    OUT[6] = (intptr_t)-1 < 0 && (int_fast16_t)-1 < 0 && (int_least32_t)-1 < 0;
    OUT[7] = 0xAA;
    return 0;
}
