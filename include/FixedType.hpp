// FixedType.hpp — the names of cc45's fixed-point types (#72): __fixed(I, F)
// is the type __fixedI_F inside the compiler (I + F bits, 16 or 32, F of
// them the fraction; __fixed8_8 / __fixed16_16 are two of them)
#pragma once
#include <cstdio>
#include <string>

// its size in bits (16 or 32), 0 for a name that is not a fixed type
inline int fixedTypeBits(const std::string& t, int* frac = nullptr) {
    if (t.rfind("__fixed", 0) != 0) return 0;
    int i = 0, f = 0;
    char extra;
    if (std::sscanf(t.c_str() + 7, "%d_%d%c", &i, &f, &extra) != 2) return 0;
    if (i < 1 || f < 1 || (i + f != 16 && i + f != 32)) return 0;
    if (frac) *frac = f;
    return i + f;
}
