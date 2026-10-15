#pragma once
// Struct/union member layout, shared by IRBuilder (code generation) and the
// ConstantFolder (sizeof / offsetof in constant expressions) so both give a
// struct the same size and offsets.
//
// cc45 packs structs: a member is aligned only by _Alignas / aligned(N) or by
// an aligned struct type. An __unpacked struct aligns each member to its
// natural alignment (a 2- or 4-byte scalar or pointer to its size, a struct to
// its own natural alignment) and is padded to a multiple of the largest one.
// Bitfields share a unit of their declared type while the type stays the same
// and the bits fit.
#include <algorithm>
#include <string>
#include <vector>

namespace StructLayout {

struct Member {
    std::string unitType;  // bitfield unit type (bitfields only)
    int size = 0;          // bytes, arrays included (a bitfield: its unit's size)
    int bitWidth = 0;      // > 0 for a bitfield
    int explicitAlign = 1; // _Alignas / aligned(N) / aligned struct type
    int naturalAlign = 1;  // used when the struct is __unpacked
};

struct Placed {
    int offset = 0;
    int bitOffset = 0;
};

struct Result {
    std::vector<Placed> placed;  // one per member
    int size = 0;
    int align = 1;          // declared alignment (packed structs align only to this)
    int naturalAlign = 1;   // alignment as an __unpacked struct's member
};

// startOffset/startAlign/startNatural: an inherited parent's size and alignments
inline Result layout(const std::vector<Member>& members, bool isUnion, bool unpacked,
                     int declAlign, int startOffset = 0, int startAlign = 1, int startNatural = 1) {
    Result r;
    r.align = std::max(1, startAlign);
    r.naturalAlign = std::max(1, startNatural);
    int offset = startOffset;
    int unionSize = 0;

    std::string bfType;
    int bfOffset = 0, bfBits = 0, bfSize = 0;
    auto closeUnit = [&]() {
        if (!bfType.empty()) {
            offset = bfOffset + bfSize;
            bfType.clear();
            bfBits = 0;
            bfSize = 0;
        }
    };
    auto alignTo = [&](int a) {
        if (a > 1 && offset % a) offset += a - offset % a;
    };

    for (const auto& m : members) {
        Placed p;
        if (m.bitWidth > 0) {
            if (bfType != m.unitType || bfBits + m.bitWidth > m.size * 8) {
                closeUnit();
                int a = unpacked ? std::max(1, m.naturalAlign) : 1;
                r.naturalAlign = std::max(r.naturalAlign, a);
                if (!isUnion) alignTo(a);
                bfType = m.unitType;
                bfOffset = isUnion ? 0 : offset;
                bfSize = m.size;
            }
            p.offset = bfOffset;
            p.bitOffset = bfBits;
            bfBits += m.bitWidth;
        } else {
            closeUnit();
            int ea = std::max(1, m.explicitAlign);
            r.align = std::max(r.align, ea);
            int a = std::max(unpacked ? std::max(1, m.naturalAlign) : 1, ea);
            r.naturalAlign = std::max(r.naturalAlign, a);
            if (isUnion) {
                p.offset = 0;
            } else {
                alignTo(a);
                p.offset = offset;
                offset += m.size;
            }
        }
        unionSize = std::max(unionSize, m.size);
        r.placed.push_back(p);
    }
    closeUnit();

    r.size = isUnion ? unionSize : offset;
    r.align = std::max(r.align, declAlign);
    r.naturalAlign = std::max(r.naturalAlign, r.align);
    int sizeAlign = unpacked ? r.naturalAlign : r.align;
    if (sizeAlign > 1 && r.size % sizeAlign) r.size += sizeAlign - r.size % sizeAlign;
    return r;
}

}  // namespace StructLayout
