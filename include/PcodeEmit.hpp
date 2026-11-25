#pragma once
// research/pcode: a function's IR as p-code (research/pcode/pcode_rt.s45 runs
// it). Not part of the toolchain: cc45 uses it only with CC45_PCODE set.
#include "IR.hpp"
#include <string>

struct PcodeResult {
    bool ok = false;
    std::string text;       // ca45 source: the frame, the native entry, the p-code
    std::string why;        // when not ok: what it does not translate
    int pcodeBytes = 0;     // the p-code's bytes
    int stubBytes = 0;      // the native entry's bytes (about)
    int frameBytes = 0;     // the frame's bytes
};

// The p-code of fn (ZP convention, no recursion: its frame is static).
PcodeResult pcodeEmit(const ir::Function& fn);
