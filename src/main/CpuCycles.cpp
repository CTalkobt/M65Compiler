// Instruction cycle counts (include/CpuCycles.hpp)
#include "CpuCycles.hpp"
#include <algorithm>
#include <map>
#include <set>

namespace cpucycles {

// the NMOS 6502: without page crossings and taken branches (JAM: 0)
const uint8_t kNmos6502[256] = {
    7,6,0,8,3,3,5,5,3,2,2,2,4,4,6,6,
    2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    6,6,0,8,3,3,5,5,4,2,2,2,4,4,6,6,
    2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    6,6,0,8,3,3,5,5,3,2,2,2,3,4,6,6,
    2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    6,6,0,8,3,3,5,5,4,2,2,2,5,4,6,6,
    2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    2,6,0,6,4,4,4,4,2,5,2,5,5,5,5,5,
    2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
    2,5,0,5,4,4,4,4,2,4,2,4,4,4,4,4,
    2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
    2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
    2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
    2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
};

namespace {

// 3.5 MHz: the 65CE02's own (xemu TIMINGS_65CE02)
const uint8_t k65CE02[256] = {
    7,5,2,2,4,3,4,4,3,2,1,1,5,4,5,4,
    2,5,5,3,4,3,4,4,1,4,1,1,5,4,5,4,
    2,5,7,7,4,3,4,4,3,2,1,1,5,4,4,4,
    2,5,5,3,4,3,4,4,1,4,1,1,5,4,5,4,
    5,5,2,2,4,3,4,4,3,2,1,1,3,4,5,4,
    2,5,5,3,4,3,4,4,1,4,3,3,4,4,5,4,
    4,5,7,5,3,3,4,4,3,2,1,1,5,4,5,4,
    2,5,5,3,3,3,4,4,2,4,3,1,5,4,5,4,
    2,5,6,3,3,3,3,4,1,2,1,4,4,4,4,4,
    2,5,5,3,3,3,3,4,1,4,1,4,4,4,4,4,
    2,5,2,2,3,3,3,4,1,2,1,4,4,4,4,4,
    2,5,5,3,3,3,3,4,1,4,1,4,4,4,4,4,
    2,5,2,6,3,3,4,4,1,2,1,7,4,4,5,4,
    2,5,5,3,3,3,4,4,1,4,3,3,4,4,5,4,
    2,5,6,6,3,3,4,4,1,2,1,6,4,4,5,4,
    2,5,5,3,5,3,4,4,1,4,3,3,7,4,5,4,
};

// 40 MHz: measured on a MEGA65 (MEGAcyc; xemu TIMINGS_65GS_FAST), without prefixes
const uint8_t k45Fast[256] = {
    9,7,3,1,6,4,6,6,3,2,1,1,7,5,7,5,
    2,7,7,4,6,4,6,6,1,5,1,1,7,5,7,5,
    5,7,8,8,4,4,6,6,4,2,1,1,5,5,7,5,
    2,7,7,4,4,4,6,6,1,5,1,1,5,5,7,5,
    6,7,1,1,6,4,6,6,3,2,1,1,5,5,7,5,
    2,7,7,4,6,4,6,6,3,5,3,1,3,5,7,5,
    5,7,5,5,3,4,6,6,4,2,1,1,7,5,7,5,
    2,7,7,4,3,4,6,6,1,5,4,1,7,5,7,5,
    4,6,7,5,3,3,3,6,1,2,1,4,4,4,4,5,
    2,6,6,4,3,3,3,6,1,4,1,4,4,4,4,5,
    2,7,2,2,4,4,4,6,1,2,1,5,5,5,5,5,
    2,7,7,4,4,4,4,6,1,5,1,5,5,5,5,5,
    2,7,2,8,4,4,6,6,1,2,1,9,5,5,7,5,
    2,7,7,4,4,4,6,6,1,5,3,3,5,5,7,5,
    2,7,8,8,4,4,6,6,1,2,1,9,5,5,7,5,
    2,7,7,4,6,4,6,6,1,5,4,4,9,5,7,5,
};

// the prefixed instructions, prefix bytes included (MEGAcyc; xemu MEGA_FOP_*)
const std::map<uint8_t, int> kQuad = {     // NEG NEG op
    {0x05, 11},
    {0x06, 17},
    {0x0A, 4},
    {0x0D, 12},
    {0x0E, 18},
    {0x12, 14},
    {0x16, 17},
    {0x1A, 4},
    {0x1E, 18},
    {0x24, 11},
    {0x25, 11},
    {0x26, 17},
    {0x2A, 4},
    {0x2C, 12},
    {0x2D, 12},
    {0x2E, 18},
    {0x32, 14},
    {0x36, 17},
    {0x3A, 4},
    {0x3E, 18},
    {0x42, 3},
    {0x43, 4},
    {0x44, 17},
    {0x45, 11},
    {0x46, 17},
    {0x4A, 4},
    {0x4D, 12},
    {0x4E, 18},
    {0x52, 14},
    {0x54, 17},
    {0x56, 17},
    {0x5E, 18},
    {0x65, 11},
    {0x66, 17},
    {0x6A, 4},
    {0x6D, 12},
    {0x6E, 18},
    {0x72, 14},
    {0x76, 17},
    {0x7E, 18},
    {0x85, 9},
    {0x8D, 10},
    {0x92, 12},
    {0xA5, 10},
    {0xAD, 11},
    {0xB2, 13},
    {0xB9, 11},
    {0xC5, 11},
    {0xC6, 17},
    {0xCD, 12},
    {0xCE, 18},
    {0xD2, 14},
    {0xD6, 17},
    {0xDE, 18},
    {0xE5, 11},
    {0xE6, 17},
    {0xED, 12},
    {0xEE, 18},
    {0xF2, 14},
    {0xF6, 17},
    {0xFE, 18},
};
const std::map<uint8_t, int> kFlat = {     // NOP (EOM) op: [$nn],z
    {0x12, 10},
    {0x32, 10},
    {0x52, 10},
    {0x72, 14},
    {0x92, 9},
    {0xB2, 13},
    {0xD2, 10},
    {0xF2, 10},
};
const std::map<uint8_t, int> kQuadFlat = { // NEG NEG NOP op
    {0x12, 17},
    {0x32, 17},
    {0x52, 17},
    {0x72, 17},
    {0x92, 15},
    {0xB2, 16},
    {0xD2, 17},
    {0xF2, 17},
};

// the 6502's 151 documented opcodes (the same instructions on the 45GS02)
const std::set<uint8_t> kDocumented = {
    0x00, 0x01, 0x05, 0x06, 0x08, 0x09, 0x0A, 0x0D, 0x0E, 0x10, 0x11, 0x15, 0x16, 0x18, 0x19, 0x1D,
    0x1E, 0x20, 0x21, 0x24, 0x25, 0x26, 0x28, 0x29, 0x2A, 0x2C, 0x2D, 0x2E, 0x30, 0x31, 0x35, 0x36,
    0x38, 0x39, 0x3D, 0x3E, 0x40, 0x41, 0x45, 0x46, 0x48, 0x49, 0x4A, 0x4C, 0x4D, 0x4E, 0x50, 0x51,
    0x55, 0x56, 0x58, 0x59, 0x5D, 0x5E, 0x60, 0x61, 0x65, 0x66, 0x68, 0x69, 0x6A, 0x6C, 0x6D, 0x6E,
    0x70, 0x71, 0x75, 0x76, 0x78, 0x79, 0x7D, 0x7E, 0x81, 0x84, 0x85, 0x86, 0x88, 0x8A, 0x8C, 0x8D,
    0x8E, 0x90, 0x91, 0x94, 0x95, 0x96, 0x98, 0x99, 0x9A, 0x9D, 0xA0, 0xA1, 0xA2, 0xA4, 0xA5, 0xA6,
    0xA8, 0xA9, 0xAA, 0xAC, 0xAD, 0xAE, 0xB0, 0xB1, 0xB4, 0xB5, 0xB6, 0xB8, 0xB9, 0xBA, 0xBC, 0xBD,
    0xBE, 0xC0, 0xC1, 0xC4, 0xC5, 0xC6, 0xC8, 0xC9, 0xCA, 0xCC, 0xCD, 0xCE, 0xD0, 0xD1, 0xD5, 0xD6,
    0xD8, 0xD9, 0xDD, 0xDE, 0xE0, 0xE1, 0xE4, 0xE5, 0xE6, 0xE8, 0xE9, 0xEA, 0xEC, 0xED, 0xEE, 0xF0,
    0xF1, 0xF5, 0xF6, 0xF8, 0xF9, 0xFD, 0xFE,
};

// at 1 / 2 MHz the 45GS02 takes a 6502's cycles for a 6502 instruction
// (the dead cycles put back); for its own, the 65CE02's, at least 2 (the
// MEGA65 Book, "Instruction Timing")
int base(Clock clock, uint8_t op) {
    switch (clock) {
        case Clock::Nmos: return kNmos6502[op];
        case Clock::Slow: return kDocumented.count(op) ? kNmos6502[op] : std::max(2, int(k65CE02[op]));
        case Clock::C65:  return k65CE02[op];
        default:          return k45Fast[op];
    }
}

} // namespace

Timing timing(Clock clock, uint8_t opcode, const std::string& mn, AddressingMode mode, int prefix) {
    Timing t;
    t.cycles = base(clock, opcode);
    if (prefix && clock != Clock::Nmos) {
        const std::map<uint8_t, int>& m = prefix == 1 ? kFlat : prefix == 2 ? kQuad : kQuadFlat;
        auto it = m.find(opcode);
        t.cycles = it != m.end() ? it->second : t.cycles + prefix;   // else a cycle per prefix byte
    }
    if (t.cycles == 0) return t;
    // page crossings and taken branches: the 6502's rules at its speeds; the
    // 65CE02 and the 45GS02 at 3.5 and 40 MHz do not add a cycle for a page
    bool sixtyFive = clock == Clock::Nmos || clock == Clock::Slow;
    static const std::set<std::string> cond8 = {"bpl", "bmi", "bvc", "bvs", "bcc", "bcs", "bne", "beq"};
    if (mode == AddressingMode::RELATIVE && cond8.count(mn)) {
        t.taken = clock == Clock::Fast ? 2 : 1;
        t.pageCross = sixtyFive;
    } else if (mode == AddressingMode::RELATIVE16 && mn != "bra" && mn != "bsr" && mn != "lbra" && mn != "lbsr") {
        t.taken = 1;
    } else if (mode == AddressingMode::BASE_PAGE_RELATIVE) {     // BBR / BBS
        t.taken = 1;
        t.pageCross = sixtyFive;
    } else if (sixtyFive && prefix == 0 &&
               (mode == AddressingMode::ABSOLUTE_X || mode == AddressingMode::ABSOLUTE_Y ||
                mode == AddressingMode::BASE_PAGE_INDIRECT_Y || mode == AddressingMode::BASE_PAGE_INDIRECT_Z)) {
        // reads only: stores and read-modify-write instructions always take the extra cycle
        static const std::set<std::string> reads = {"lda", "ldx", "ldy", "ldz", "adc", "sbc", "and", "ora", "eor",
                                                    "cmp", "bit", "lax", "las", "nop"};
        t.pageCross = reads.count(mn) > 0;
    }
    return t;
}

std::string format(const Timing& t) {
    if (t.cycles == 0) return "?";
    std::string s = std::to_string(t.cycles);
    if (t.taken) s += "/" + std::to_string(t.cycles + t.taken);
    if (t.pageCross) s += "*";
    return s;
}

} // namespace cpucycles
