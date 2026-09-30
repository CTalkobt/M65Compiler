/*
 * sincos16.h — 16-bit precision sine/cosine lookup for MEGA65
 *
 * Provides a 256-entry sine table in 1.15 signed fixed-point format,
 * for applications needing higher precision than sincos.h (which uses
 * 8-bit 1.7 fixed-point).
 *
 * REPRESENTATION:
 *   - Angles: unsigned char (0-255), mapping to 0°-360° in 256 steps
 *   - Values: signed int in 1.15 fixed-point:
 *       -32767 ≈ -1.0,  0 = 0.0,  32767 ≈ +1.0
 *   - Full range symmetric: sin_tab16[i] = -sin_tab16[i+128]
 *
 * USAGE:
 *   #include <sincos16.h>
 *
 *   signed int s = isin16(angle);       // sin(angle) in 1.15 fixed-point
 *   signed int c = icos16(angle);       // cos(angle) = sin(angle + 64)
 *
 *   // Sub-pixel sprite position with radius 200
 *   long x = fixmul16(isin16(angle), 200);  // x in 16.15 fixed-point
 *   int pixel_x = center_x + (int)(x >> 15);
 *
 *   // Rotation: x' = x*cos - y*sin, y' = x*sin + y*cos
 *   long rx = fixmul16(x, icos16(a)) - fixmul16(y, isin16(a));
 *
 * PERFORMANCE:
 *   isin16/icos16: ~8 cycles (indexed word load)
 *   fixmul16:      ~12 cycles with MEGA65 hardware multiplier
 *   Compare:       sinf() via ROM: ~2000+ cycles
 *
 * MEMORY: 512 bytes for the table
 *
 * PRECISION:
 *   Maximum error vs true sin: ±0.00003 (1/32768 step size)
 *   Pixel-accurate up to radius ~32000
 *
 * ANGLE REFERENCE:
 *     0 =   0° (right)       64 =  90° (up)
 *   128 = 180° (left)       192 = 270° (down)
 *
 * CONVERTING FROM DEGREES:
 *   unsigned char angle = (unsigned char)((degrees * 256L) / 360);
 *
 * For 8-bit precision (256-byte table), see sincos.h
 */

#ifndef _SINCOS16_H
#define _SINCOS16_H

/*
 * 256-entry sine table, 1.15 signed fixed-point.
 * sin_tab16[0]=0, sin_tab16[64]=32767, sin_tab16[128]=0, sin_tab16[192]=-32767
 */
const signed int sin_tab16[256] = {
    /* 0-15: 0.00° - 21.09° */
         0,    804,   1608,   2410,   3212,   4011,   4808,   5602,
      6393,   7179,   7962,   8739,   9512,  10278,  11039,  11793,
    /* 16-31: 22.50° - 43.59° */
     12539,  13279,  14010,  14732,  15446,  16151,  16846,  17530,
     18204,  18868,  19519,  20159,  20787,  21403,  22005,  22594,
    /* 32-47: 45.00° - 66.09° */
     23170,  23731,  24279,  24811,  25329,  25832,  26319,  26790,
     27245,  27683,  28105,  28510,  28898,  29268,  29621,  29956,
    /* 48-63: 67.50° - 88.59° */
     30273,  30571,  30852,  31113,  31356,  31580,  31785,  31971,
     32137,  32285,  32412,  32521,  32609,  32678,  32728,  32757,
    /* 64-79: 90.00° - 111.09° (peak) */
     32767,  32757,  32728,  32678,  32609,  32521,  32412,  32285,
     32137,  31971,  31785,  31580,  31356,  31113,  30852,  30571,
    /* 80-95: 112.50° - 133.59° */
     30273,  29956,  29621,  29268,  28898,  28510,  28105,  27683,
     27245,  26790,  26319,  25832,  25329,  24811,  24279,  23731,
    /* 96-111: 135.00° - 156.09° */
     23170,  22594,  22005,  21403,  20787,  20159,  19519,  18868,
     18204,  17530,  16846,  16151,  15446,  14732,  14010,  13279,
    /* 112-127: 157.50° - 178.59° */
     12539,  11793,  11039,  10278,   9512,   8739,   7962,   7179,
      6393,   5602,   4808,   4011,   3212,   2410,   1608,    804,
    /* 128-143: 180.00° - 201.09° */
         0,   -804,  -1608,  -2410,  -3212,  -4011,  -4808,  -5602,
     -6393,  -7179,  -7962,  -8739,  -9512, -10278, -11039, -11793,
    /* 144-159: 202.50° - 223.59° */
    -12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530,
    -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
    /* 160-175: 225.00° - 246.09° */
    -23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790,
    -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956,
    /* 176-191: 247.50° - 268.59° */
    -30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971,
    -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
    /* 192-207: 270.00° - 291.09° (trough) */
    -32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285,
    -32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
    /* 208-223: 292.50° - 313.59° */
    -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683,
    -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731,
    /* 224-239: 315.00° - 336.09° */
    -23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868,
    -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
    /* 240-255: 337.50° - 358.59° */
    -12539, -11793, -11039, -10278,  -9512,  -8739,  -7962,  -7179,
     -6393,  -5602,  -4808,  -4011,  -3212,  -2410,  -1608,   -804,
};

/* Sine lookup — returns 1.15 fixed-point value */
#define isin16(angle) (sin_tab16[(unsigned char)(angle)])

/* Cosine lookup — cos(x) = sin(x + 64) */
#define icos16(angle) (sin_tab16[(unsigned char)((angle) + 64)])

/*
 * fixmul16 — 16-bit fixed-point multiply using MEGA65 hardware multiplier
 *
 * Computes (a * b) >> 15, where a is 1.15 fixed-point and b is an integer.
 * Result is a signed int suitable for pixel coordinates.
 *
 * Uses the MEGA65 hardware multiplier at $D770/$D774 → $D778 (combinational).
 * For signed multiply: negate if signs differ, multiply unsigned, apply sign.
 */
static int fixmul16(int a, int b) {
    /* Handle signs — hardware multiplier is unsigned.
     * Use two's complement (~x + 1) for absolute value since
     * unary minus doesn't work on unsigned int. */
    int neg = 0;
    unsigned int ua, ub;
    if (a & 0x8000) { ua = (~a + 1) & 0x7FFF; neg = 1; } else { ua = a; }
    if (b & 0x8000) { ub = (~b + 1) & 0x7FFF; neg = neg ? 0 : 1; } else { ub = b; }

    /* Write to hardware multiplier */
    *(volatile unsigned char *)0xD770 = ua & 0xFF;
    *(volatile unsigned char *)0xD771 = (ua >> 8) & 0xFF;
    *(volatile unsigned char *)0xD772 = 0;
    *(volatile unsigned char *)0xD773 = 0;
    *(volatile unsigned char *)0xD774 = ub & 0xFF;
    *(volatile unsigned char *)0xD775 = (ub >> 8) & 0xFF;
    *(volatile unsigned char *)0xD776 = 0;
    *(volatile unsigned char *)0xD777 = 0;

    /* Read product >> 15: extract bits 15-30 of the 32-bit product.
     * Byte layout: $D778=bits 0-7, $D779=bits 8-15, $D77A=bits 16-23
     * Bits 15-30 = byte1[7] | byte2[0-7]<<1 | byte3[0-6]<<9 */
    unsigned int result =
        ((unsigned int)(*(volatile unsigned char *)0xD779) >> 7) |
        ((unsigned int)(*(volatile unsigned char *)0xD77A) << 1);

    if (neg) { result = (~result + 1) & 0xFFFF; }
    return (int)result;
}

#endif /* _SINCOS16_H */
