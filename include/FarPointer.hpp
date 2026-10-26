#pragma once
// Far and near pointers (#68): T __far *p is a 4-byte value holding a
// 28-bit address, a scalar type cc45 names "__farp T"; T __near *p a 1-byte
// offset into the program's base page, "__nearp T" ("__farp signed T" /
// "__nearp signed T" for a signed T). The levels after the first '*' are
// plain pointers to it.
#include <string>

inline bool isFarPointerType(const std::string& t) { return t.rfind("__farp ", 0) == 0; }
inline bool isNearPointerType(const std::string& t) { return t.rfind("__nearp ", 0) == 0; }

// The type a far or near pointer points to ("" for another type), and its
// signedness
inline std::string farPointee(const std::string& t, bool* isSigned = nullptr) {
    std::string p;
    if (isFarPointerType(t)) p = t.substr(7);
    else if (isNearPointerType(t)) p = t.substr(8);
    else return "";
    bool sg = p.rfind("signed ", 0) == 0;
    if (sg) p = p.substr(7);
    if (isSigned) *isSigned = sg;
    return p;
}
