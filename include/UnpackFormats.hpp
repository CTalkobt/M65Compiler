#pragma once
// The pack45 stream formats a file unpacks where the stream is visible
// (#666): unpack() / unpack_to() of a byte array whose initializer is in the
// file (#embed too), load_unpack() of a literal name whose file is found
// beside the source or in an include directory. cl45's mega65-unpack-format
// compares them with the decoders the file links (UNPACK_FORMATn), and cc45
// records them in the object (cc45.unpackfmt.<format>.<line>.<file in hex>)
// for ln45 to compare with the decoders the program links.

#include <set>
#include <string>
#include <vector>

class TranslationUnit;

namespace unpackfmt {

struct Site {
    int format = 0;             // the stream's format byte: 1 or 2
    std::string file;           // the call
    int line = 0, column = 0;
    std::string what;           // the stream: "array 'x'", "file tiles.prg"
};

// the calls of the unit whose stream's format is known (a stream whose
// format byte is not 1 or 2, or cannot be read, is left out)
std::vector<Site> find(TranslationUnit& unit, const std::vector<std::string>& dirs);

// the formats whose decoders file-scope asm statements name
// (___unpack_format<n>: <unpack.h>'s UNPACK_FORMATn, and its default)
std::set<int> declared(const std::vector<std::string>& fileAsm);

// the record cc45 writes for a site, and the site back from one (false: not
// one); here, so ln45 needs none of the rest
inline std::string recordName(const Site& s) {
    static const char* hex = "0123456789abcdef";
    std::string h;
    for (unsigned char c : s.file) { h += hex[c >> 4]; h += hex[c & 15]; }
    return "cc45.unpackfmt." + std::to_string(s.format) + "." + std::to_string(s.line) + "." + h;
}

inline bool parseRecord(const std::string& name, Site& s) {
    const std::string pre = "cc45.unpackfmt.";
    if (name.compare(0, pre.size(), pre) != 0) return false;
    size_t a = pre.size(), b = name.find('.', a), c = b == std::string::npos ? b : name.find('.', b + 1);
    if (c == std::string::npos) return false;
    try {
        s.format = std::stoi(name.substr(a, b - a));
        s.line = std::stoi(name.substr(b + 1, c - b - 1));
    } catch (...) { return false; }
    std::string h = name.substr(c + 1), f;
    if (h.size() % 2) return false;
    for (size_t i = 0; i < h.size(); i += 2) {
        auto v = [](char ch) { return ch >= 'a' ? ch - 'a' + 10 : ch - '0'; };
        f += (char)(v(h[i]) << 4 | v(h[i + 1]));
    }
    s.file = f;
    return true;
}


}  // namespace unpackfmt
