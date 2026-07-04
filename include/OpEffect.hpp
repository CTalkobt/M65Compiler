#pragma once
#include "MachineState.hpp"
#include <cstdint>
#include <string>

// Centralized instruction effect table.
// Single source of truth for what each instruction (native or simulated) clobbers.
// Keyed by mnemonic string so both the compiler (IRCodeGen) and assembler
// (AssemblerOptimizer) can use the same table.

struct OpEffect {
    uint8_t clobbers = 0;      // bitmask of clobbered registers
    bool clobbersFlags = false;
    bool modifiesMem = false;   // writes to ZP, absolute, or stack memory
    bool modifiesStack = false; // changes SP (push/pull)

    // Register bitmask constants
    static constexpr uint8_t A  = (1 << REG_A);
    static constexpr uint8_t X  = (1 << REG_X);
    static constexpr uint8_t Y  = (1 << REG_Y);
    static constexpr uint8_t Z  = (1 << REG_Z);
    static constexpr uint8_t SP = (1 << REG_SP);
    static constexpr uint8_t ALL = A | X | Y | Z | SP;

    // Apply this effect to a MachineState
    void apply(MachineState& ms) const {
        for (int i = 0; i < REG_COUNT; i++) {
            if (clobbers & (1 << i)) ms.invalidateReg((RegId)i);
        }
        if (clobbersFlags) ms.flags.invalidate();
        if (modifiesMem) {
            ms.invalidateAllMem();
            ms.invalidateAllStack();
        }
        if (modifiesStack) ms.spModified();
    }
};

// Look up the effect of any instruction by mnemonic string.
// Works for both native instructions ("lda", "tsx", "pha") and
// simulated ops ("stax.fp", "add.16", "mul.s16").
// Returns a conservative "clobber everything" for unknown mnemonics.
const OpEffect& getOpEffect(const std::string& mnemonic);
