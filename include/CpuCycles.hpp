#pragma once
// Instruction cycle counts (objdump45 --cycles, #144; tune45's 6502 emulation)
//
// - the NMOS 6502 / 6510: the standard table (all 256 opcodes; JAM 0), as the
//   MEGA65 Book's 6502 instruction set lists them;
// - the 45GS02 at 1 / 2 MHz: a 6502 instruction the 6502's count (the dead
//   cycles put back), its own instructions the 65CE02's, at least 2;
// - the 45GS02 at 3.5 MHz, and the 65CE02: the 65CE02's own (xemu's TIMINGS_65CE02);
// - the 45GS02 at 40 MHz: measured on a MEGA65 by LGB's MEGAcyc
//   (https://github.com/lgblgblgb/megacyc; xemu's TIMINGS_65GS_FAST and
//   MEGA_FOP_* for the instructions with NEG NEG / NOP prefixes, used at every
//   speed). The MEGA65 Book's own 40 MHz table has no counts for opcodes $80-$FF.
#include <cstdint>
#include <string>
#include "AssemblerTypes.hpp"

namespace cpucycles {

enum class Clock {
    Nmos,   // the 6502 / 6510
    Slow,   // the 45GS02 at 1 or 2 MHz
    C65,    // the 45GS02 at 3.5 MHz; the 65CE02
    Fast    // the 45GS02 at 40 MHz
};

extern const uint8_t kNmos6502[256];

struct Timing {
    int cycles = 0;         // without a taken branch or a page crossing; 0: unknown (JAM)
    int taken = 0;          // added when a branch is taken
    bool pageCross = false; // one more when the indexed address or the branch crosses a page
};

// The timing of one instruction: its opcode, its mnemonic (lower case) and
// mode as decoded, and its prefix bytes: 0 none, 1 NOP ($EA: [$nn],z),
// 2 NEG NEG (a quad instruction), 3 NEG NEG NOP
Timing timing(Clock clock, uint8_t opcode, const std::string& mnemonic, AddressingMode mode, int prefix = 0);

// "4", "4*" (+1 when a page is crossed), "2/3*" (not taken / taken), "?" (unknown)
std::string format(const Timing& t);

} // namespace cpucycles
