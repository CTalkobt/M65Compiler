#pragma once
#include <cmath>
#include <cstdint>

// Convert an IEEE 754 double to the CBM 40-bit float format (5 bytes:
// exponent, then mantissa MSB first with the sign in bit 7 of the first
// mantissa byte) used by BASIC 65's floating point routines.
inline void doubleToCBM40(double val, uint8_t out[5]) {
    if (val == 0.0) {
        // Exponent 0 is zero; -0.0 keeps its sign bit (signbit, copysign)
        out[0]=out[1]=out[2]=out[3]=out[4]=0;
        if (std::signbit(val)) out[1] = 0x80;
        return;
    }
    bool neg = val < 0; if (neg) val = -val;
    int exp; double mant = std::frexp(val, &exp);
    // Round the 32-bit mantissa to nearest (truncating made 1e20 one unit
    // low, which BASIC's FOUT then prints as " 0E19"); 0.99999999998 can
    // round up to 2^32: that is 0.5 with the exponent one higher
    uint64_t m64 = (uint64_t)std::llround(mant * 4294967296.0);
    if (m64 >= 4294967296ULL) { m64 = 2147483648ULL; exp++; }
    int cbmExp = exp + 128;
    if (cbmExp < 0) cbmExp = 0; if (cbmExp > 255) cbmExp = 255;
    out[0] = (uint8_t)cbmExp;
    uint32_t m = (uint32_t)m64;
    out[1] = (uint8_t)(m >> 24); out[2] = (uint8_t)(m >> 16);
    out[3] = (uint8_t)(m >> 8);  out[4] = (uint8_t)(m);
    if (neg) out[1] |= 0x80; else out[1] &= 0x7F;
}
