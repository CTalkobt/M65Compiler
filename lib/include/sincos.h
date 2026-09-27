/*
 * sincos.h — Fast integer sine/cosine lookup table for MEGA65
 *
 * Provides a 256-entry sine lookup table using 1.7 signed fixed-point
 * representation, suitable for real-time graphics, sprite movement,
 * scrolling, and demo effects where floating-point ROM calls are too slow.
 *
 * REPRESENTATION:
 *   - Angles are unsigned char (0-255), mapping to 0°-360° in 256 steps
 *     (1.40625° per step). Wraps naturally — no bounds checking needed.
 *   - Values are signed char in 1.7 fixed-point:
 *       -128 ≈ -1.0,  0 = 0.0,  127 ≈ +0.992
 *     Actual range is -127 to +127 (symmetric).
 *
 * USAGE:
 *   #include <sincos.h>
 *
 *   // Basic lookup
 *   signed char s = isin(angle);     // sin(angle)
 *   signed char c = icos(angle);     // cos(angle) = sin(angle + 64)
 *
 *   // Compute pixel offset from angle and radius
 *   int x = fixmul(isin(angle), radius);   // x = sin(angle) * radius
 *   int y = fixmul(icos(angle), radius);   // y = cos(angle) * radius
 *
 *   // Circle motion example
 *   unsigned char angle = 0;
 *   while (1) {
 *       int sx = center_x + fixmul(isin(angle), 50);
 *       int sy = center_y + fixmul(icos(angle), 50);
 *       // ... draw sprite at (sx, sy) ...
 *       angle++;  // wraps 0-255 automatically
 *   }
 *
 *   // Lissajous pattern
 *   int x = fixmul(isin(angle * 3), 80);
 *   int y = fixmul(icos(angle * 2), 60);
 *
 * PERFORMANCE:
 *   isin/icos: ~6 cycles (single indexed load)
 *   fixmul:    ~20 cycles with hardware multiplier, ~80 without
 *   Compare:   sinf() via ROM: ~2000+ cycles
 *
 * MEMORY: 256 bytes for the table (in .data/.rodata segment)
 *
 * PRECISION:
 *   Maximum error vs true sin: ±0.008 (1/128 step size)
 *   Sufficient for pixel-level accuracy up to radius ~160
 *
 * ANGLE REFERENCE:
 *     0 =   0° (right)       64 =  90° (up)
 *   128 = 180° (left)       192 = 270° (down)
 *
 * CONVERTING FROM DEGREES:
 *   unsigned char angle = (unsigned char)((degrees * 256L) / 360);
 *   // Or approximate: angle ≈ (degrees * 91) >> 5;
 *
 * NOTES:
 *   - cos(x) = sin(x + 64), so only one table is needed
 *   - The table is symmetric: sin(128+x) = -sin(x)
 *   - For quarter-table (64-byte) variant, use isin_q()/icos_q()
 *   - For 16-bit precision (8.8 fixed-point), see sincos16.h (planned)
 */

#ifndef _SINCOS_H
#define _SINCOS_H

/*
 * 256-entry sine table, 1.7 signed fixed-point.
 * sin_tab[0]=sin(0°)=0, sin_tab[64]=sin(90°)=127, sin_tab[128]=sin(180°)=0
 */
const signed char sin_tab[256] = {
    /* 0-15: 0.00° - 21.09° */
       0,    3,    6,    9,   12,   16,   19,   22,
      25,   28,   31,   34,   37,   40,   43,   46,
    /* 16-31: 22.50° - 43.59° */
      49,   51,   54,   57,   59,   62,   65,   67,
      70,   73,   75,   78,   80,   82,   85,   87,
    /* 32-47: 45.00° - 66.09° */
      89,   91,   94,   96,   98,  100,  102,  103,
     105,  107,  108,  110,  112,  113,  114,  116,
    /* 48-63: 67.50° - 88.59° */
     117,  118,  119,  120,  121,  122,  123,  123,
     124,  125,  125,  126,  126,  126,  127,  127,
    /* 64-79: 90.00° - 111.09° */
     127,  127,  127,  126,  126,  126,  125,  125,
     124,  123,  123,  122,  121,  120,  119,  118,
    /* 80-95: 112.50° - 133.59° */
     117,  116,  114,  113,  112,  110,  108,  107,
     105,  103,  102,  100,   98,   96,   94,   91,
    /* 96-111: 135.00° - 155.39° */
      89,   87,   85,   82,   80,   78,   75,   73,
      70,   67,   65,   62,   59,   57,   54,   51,
    /* 112-127: 157.50° - 178.59° */
      49,   46,   43,   40,   37,   34,   31,   28,
      25,   22,   19,   16,   12,    9,    6,    3,
    /* 128-143: 180.00° - 201.09° */
       0,   -3,   -6,   -9,  -12,  -16,  -19,  -22,
     -25,  -28,  -31,  -34,  -37,  -40,  -43,  -46,
    /* 144-159: 202.50° - 223.59° */
     -49,  -51,  -54,  -57,  -59,  -62,  -65,  -67,
     -70,  -73,  -75,  -78,  -80,  -82,  -85,  -87,
    /* 160-175: 225.00° - 246.09° */
     -89,  -91,  -94,  -96,  -98, -100, -102, -103,
    -105, -107, -108, -110, -112, -113, -114, -116,
    /* 176-191: 247.50° - 268.59° */
    -117, -118, -119, -120, -121, -122, -123, -123,
    -124, -125, -125, -126, -126, -126, -127, -127,
    /* 192-207: 270.00° - 291.09° */
    -127, -127, -127, -126, -126, -126, -125, -125,
    -124, -123, -123, -122, -121, -120, -119, -118,
    /* 208-223: 292.50° - 313.59° */
    -117, -116, -114, -113, -112, -110, -108, -107,
    -105, -103, -102, -100,  -98,  -96,  -94,  -91,
    /* 224-239: 315.00° - 335.39° */
     -89,  -87,  -85,  -82,  -80,  -78,  -75,  -73,
     -70,  -67,  -65,  -62,  -59,  -57,  -54,  -51,
    /* 240-255: 337.50° - 358.59° */
     -49,  -46,  -43,  -40,  -37,  -34,  -31,  -28,
     -25,  -22,  -19,  -16,  -12,   -9,   -6,   -3,
};

/* Sine lookup: angle 0-255 → signed char -127..+127 (1.7 fixed-point) */
#define isin(angle) (sin_tab[(unsigned char)(angle)])

/* Cosine lookup: cos(x) = sin(x + 64) */
#define icos(angle) (sin_tab[(unsigned char)((angle) + 64)])

/*
 * Fixed-point multiply: (1.7 value) * integer → integer result.
 * Computes (a * b) >> 7, correctly handling signed 1.7 values.
 *
 * Usage: int offset = fixmul(isin(angle), radius);
 *
 * For positive radius up to 255, this gives pixel-accurate results.
 * Uses signed multiply internally; result is a signed int.
 */
static inline signed int fixmul(signed char a, signed int b) {
    return ((signed int)a * b) >> 7;
}

/*
 * Quarter-table variants (saves 192 bytes, costs ~10 extra cycles).
 * Use when RAM is tight. Derives full sin/cos from 64-entry table.
 */

/* Quarter sine table: 0° to 90° only (64 entries) */
const signed char sin_tab_q[64] = {
       0,    3,    6,    9,   12,   16,   19,   22,
      25,   28,   31,   34,   37,   40,   43,   46,
      49,   51,   54,   57,   59,   62,   65,   67,
      70,   73,   75,   78,   80,   82,   85,   87,
      89,   91,   94,   96,   98,  100,  102,  103,
     105,  107,  108,  110,  112,  113,  114,  116,
     117,  118,  119,  120,  121,  122,  123,  123,
     124,  125,  125,  126,  126,  126,  127,  127,
};

/* Quarter-table sine: derives full period from 64-entry table */
static inline signed char isin_q(unsigned char angle) {
    unsigned char quadrant = angle >> 6;
    unsigned char idx = angle & 63;
    if (quadrant & 1) idx = 63 - idx;       /* mirror in odd quadrants */
    signed char val = sin_tab_q[idx];
    return (quadrant >= 2) ? -val : val;     /* negate in lower half */
}

/* Quarter-table cosine */
static inline signed char icos_q(unsigned char angle) {
    return isin_q(angle + 64);
}

#endif /* _SINCOS_H */
