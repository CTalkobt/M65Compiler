#include "Lz77.hpp"

namespace lz77 {

namespace {

// A variable-size number: 7 bits per byte, most significant first
bool readVarSize(const std::vector<uint8_t>& in, size_t& pos, uint32_t& v) {
    v = 0;
    for (int n = 0; n < 5; n++) {
        if (pos >= in.size()) return false;
        uint8_t b = in[pos++];
        v = (v << 7) | (b & 0x7F);
        if (!(b & 0x80)) return true;
    }
    return false;
}

}  // namespace

bool decompress(const std::vector<uint8_t>& in, std::vector<uint8_t>& out, size_t maxOut) {
    out.clear();
    if (in.empty()) return false;
    uint8_t marker = in[0];
    size_t pos = 1;
    while (pos < in.size()) {
        uint8_t sym = in[pos++];
        if (sym != marker) {
            out.push_back(sym);
        } else {
            if (pos >= in.size()) return false;
            if (in[pos] == 0) {
                out.push_back(marker);
                pos++;
            } else {
                uint32_t length, offset;
                if (!readVarSize(in, pos, length) || !readVarSize(in, pos, offset)) return false;
                if (offset == 0 || offset > out.size()) return false;
                for (uint32_t i = 0; i < length; i++) out.push_back(out[out.size() - offset]);
            }
        }
        if (out.size() > maxOut) return false;
    }
    return true;
}

}  // namespace lz77
