#pragma once
// Far and near pointers (#68): T __far *p is a 4-byte value holding a
// 28-bit address, a scalar type cc45 names "__farp T"; T __near *p a 1-byte
// offset into the program's base page, "__nearp T" ("__farp signed T" /
// "__nearp signed T" for a signed T). The levels after the first '*' are
// plain pointers to it.
#include <string>
#include <vector>

inline bool isFarPointerType(const std::string& t) { return t.rfind("__farp ", 0) == 0; }
inline bool isNearPointerType(const std::string& t) { return t.rfind("__nearp ", 0) == 0; }

// A far pointer to an array (#347) names its pointee with the dimensions:
// "__farp T[N]" (T (__far *p)[N], or a typedef'd array: pair __far *p).
// farArrayDims takes them off the pointee's name, in order.
inline std::vector<int> farArrayDims(std::string& pointee) {
    std::vector<int> dims;
    size_t b = pointee.find('[');
    if (b == std::string::npos) return dims;
    for (size_t i = b; i < pointee.size() && pointee[i] == '[';) {
        size_t e = pointee.find(']', i);
        if (e == std::string::npos) break;
        dims.push_back(std::stoi(pointee.substr(i + 1, e - i - 1)));
        i = e + 1;
    }
    pointee.erase(b);
    while (!pointee.empty() && pointee.back() == ' ') pointee.pop_back();
    return dims;
}
// The pointee's name with array dimensions after it
inline std::string farArrayType(const std::string& type, const std::vector<int>& dims) {
    std::string t = type;
    for (int d : dims) t += "[" + std::to_string(d) + "]";
    return t;
}

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
