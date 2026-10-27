// KnownBits.cpp — known-zero bits of a function's values (#350); see
// KnownBits.hpp.
#include "KnownBits.hpp"
#include <set>

namespace ir {

int KnownBits::width(Type t) {
    switch (t) {
        case Type::I8: return 8;
        case Type::I16: case Type::PTR: return 16;
        case Type::I32: return 32;
        default: return 64;
    }
}

uint64_t KnownBits::of(const Operand& o, int w) const {
    uint64_t m = mask(w);
    if (o.isImm()) return ~(uint64_t)o.immVal & m;
    if (o.isVreg()) {
        auto it = zero.find(o.vregId);
        // (an operand narrower than the use: its bits above are not known)
        if (it != zero.end()) return it->second & m & mask(width(o.type) < w ? width(o.type) : w);
    }
    return 0;
}

static int trailingZeros(uint64_t kz) {
    int n = 0;
    while (n < 64 && ((kz >> n) & 1)) n++;
    return n;
}

static int bitLength(uint64_t v) {
    int n = 0;
    while (v) { n++; v >>= 1; }
    return n;
}

KnownBits computeKnownBits(const Function& fn) {
    KnownBits kb;
    std::map<uint32_t, int> defs;
    for (const auto& b : fn.blocks)
        for (const auto& in : b.insts)
            if (in.dest.isVreg() && in.op != Op::STORE) defs[in.dest.vregId]++;
    auto known = [&](uint32_t v) {
        return defs[v] == 1 && !fn.localSlotVregs.count(v) && !fn.memoryVregs.count(v) && v >= fn.paramTypes.size();
    };
    // a local assigned once (one store, nothing else writing it, its
    // address not taken): its value's bits (#350: c = 0x1010 + k * 4, then
    // c + 1). A read before the store reads an undefined value.
    std::map<uint32_t, int> slotWrites;
    std::map<uint32_t, Operand> slotValue;
    std::set<uint32_t> addressed;
    for (const auto& b : fn.blocks)
        for (const auto& in : b.insts) {
            if (in.op == Op::STORE && in.src2.isVreg() && in.args.empty() && fn.localSlotVregs.count(in.src2.vregId)) {
                slotWrites[in.src2.vregId]++;
                slotValue[in.src2.vregId] = in.src1;
            } else if (in.op == Op::STORE && in.src2.isVreg() && fn.localSlotVregs.count(in.src2.vregId)) {
                slotWrites[in.src2.vregId] += 2;   // (a fused element store: not the slot's value)
            }
            if (in.dest.isVreg() && in.op != Op::STORE && fn.localSlotVregs.count(in.dest.vregId)) slotWrites[in.dest.vregId] += 2;
            if (in.op == Op::ADDR_LOCAL && in.src1.isVreg()) addressed.insert(in.src1.vregId);
        }
    for (auto it = slotValue.begin(); it != slotValue.end();) {
        uint32_t v = it->first;
        bool ok = slotWrites[v] == 1 && !addressed.count(v) && !fn.memoryVregs.count(v) && v >= fn.paramTypes.size() &&
                  !fn.vregSizes.count(v);
        it = ok ? std::next(it) : slotValue.erase(it);
    }
    auto slotType = [&](uint32_t v) {
        auto t = fn.vregTypes.find(v);
        return t == fn.vregTypes.end() ? Type::VOID : t->second;
    };
    // (no cycle through single definitions but a loop's own value: an
    // iteration from "nothing known" only adds facts the operands give)
    for (int round = 0; round < 8; round++) {
        bool changed = false;
        for (const auto& b : fn.blocks)
            for (const auto& in : b.insts) {
                if (!in.dest.isVreg() || in.op == Op::STORE || !known(in.dest.vregId)) continue;
                int w = KnownBits::width(in.resultType);
                if (w >= 64) continue;
                uint64_t M = KnownBits::mask(w), z = 0;
                auto shiftOf = [&]() { return in.src2.isImm() ? (int)(in.src2.immVal & 0x3F) : -1; };
                switch (in.op) {
                    case Op::CONST:
                        z = ~(uint64_t)in.src1.immVal & M;
                        break;
                    case Op::COPY:
                        if (KnownBits::width(in.src1.type) == w || in.src1.isImm()) z = kb.of(in.src1, w);
                        break;
                    case Op::ZEXT: {
                        int sw = KnownBits::width(in.src1.type);
                        if (sw < w) z = (kb.of(in.src1, sw) | ~KnownBits::mask(sw)) & M;
                        break;
                    }
                    case Op::TRUNC:
                        z = kb.of(in.src1, w);
                        break;
                    case Op::AND:
                        z = kb.of(in.src1, w) | kb.of(in.src2, w);
                        break;
                    case Op::OR: case Op::XOR:
                        z = kb.of(in.src1, w) & kb.of(in.src2, w);
                        break;
                    case Op::SHL: case Op::LSL: {
                        int k = shiftOf();
                        if (k >= 0) z = ((kb.of(in.src1, w) << k) | KnownBits::mask(k)) & M;
                        break;
                    }
                    case Op::SHR: case Op::LSR: {
                        int k = shiftOf();
                        if (k >= 0) z = ((kb.of(in.src1, w) >> k) | (~(M >> k) & M)) & M;
                        break;
                    }
                    case Op::MUL: case Op::MUL_U: {
                        // the trailing zeros add up; the product's length bounds it
                        int t = std::min(64, trailingZeros(kb.of(in.src1, w)) + trailingZeros(kb.of(in.src2, w)));
                        z = KnownBits::mask(t > w ? w : t);
                        unsigned __int128 p = (unsigned __int128)kb.maxOf(in.src1, w) * kb.maxOf(in.src2, w);
                        if (p < ((unsigned __int128)1 << w)) z |= ~KnownBits::mask(bitLength((uint64_t)p)) & M;
                        break;
                    }
                    case Op::ADD: {
                        int t = std::min(trailingZeros(kb.of(in.src1, w)), trailingZeros(kb.of(in.src2, w)));
                        z = KnownBits::mask(t > w ? w : t);
                        unsigned __int128 s = (unsigned __int128)kb.maxOf(in.src1, w) + kb.maxOf(in.src2, w);
                        if (s < ((unsigned __int128)1 << w)) z |= ~KnownBits::mask(bitLength((uint64_t)s)) & M;
                        break;
                    }
                    default:
                        break;
                }
                z &= M;
                auto it = kb.zero.find(in.dest.vregId);
                if (it == kb.zero.end() ? z != 0 : it->second != z) { kb.zero[in.dest.vregId] = z; changed = true; }
            }
        for (const auto& [v, val] : slotValue) {
            int w = KnownBits::width(slotType(v));
            if (w >= 64) continue;
            uint64_t z = kb.of(val, w) & KnownBits::mask(w);
            auto it = kb.zero.find(v);
            if (it == kb.zero.end() ? z != 0 : it->second != z) { kb.zero[v] = z; changed = true; }
        }
        if (!changed) break;
    }
    for (auto it = kb.zero.begin(); it != kb.zero.end();) it = it->second ? std::next(it) : kb.zero.erase(it);
    return kb;
}

}  // namespace ir
