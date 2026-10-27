#pragma once
// KnownBits.hpp — the bits of a function's values known to be 0 (#350):
// (unsigned)y << 9 has a zero low byte, x << 2 of a char fits 10 bits,
// k * 4 has its low two bits clear. Code generation works byte by byte
// where a byte is known 0 and leaves out carries that cannot happen.
//
// Only vregs defined once and not a local's slot (the IR is not SSA: a
// slot is assigned by store, a temporary can have several definitions);
// anything else is unknown (no bit known).
#include "IR.hpp"
#include <cstdint>
#include <map>

namespace ir {

struct KnownBits {
    std::map<uint32_t, uint64_t> zero;   // vreg -> its bits known 0 (within its width)

    static int width(Type t);            // 8, 16, 32 (64: unknown types)
    static uint64_t mask(int w) { return w >= 64 ? ~0ULL : ((1ULL << w) - 1); }
    // an operand's known-zero bits: an immediate's exactly, a vreg's, else none
    uint64_t of(const Operand& o, int w) const;
    // the largest value the operand can have (the bits not known 0)
    uint64_t maxOf(const Operand& o, int w) const { return ~of(o, w) & mask(w); }
    // byte k of the operand is known 0
    bool byteZero(const Operand& o, int w, int k) const { return ((of(o, w) >> (8 * k)) & 0xFF) == 0xFF; }
};

KnownBits computeKnownBits(const Function& fn);

}  // namespace ir
