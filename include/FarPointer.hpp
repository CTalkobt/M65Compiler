#pragma once
// Far pointers (#68): T __far *p is a 4-byte value holding a 28-bit address,
// a scalar type cc45 names "__farp T" ("__farp signed T" for a signed T);
// the levels after the first '*' are plain pointers to it.
#include <string>

inline bool isFarPointerType(const std::string& t) { return t.rfind("__farp ", 0) == 0; }

// The type a far pointer points to ("" for another type), and its signedness
inline std::string farPointee(const std::string& t, bool* isSigned = nullptr) {
    if (!isFarPointerType(t)) return "";
    std::string p = t.substr(7);
    bool sg = p.rfind("signed ", 0) == 0;
    if (sg) p = p.substr(7);
    if (isSigned) *isSigned = sg;
    return p;
}
