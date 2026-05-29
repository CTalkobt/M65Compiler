#pragma once
#include <cstdint>

// Register & Memory Value Tracking Framework — Phase 1
// Tracks what is known about CPU register values during code emission.
// Replaces ad-hoc flags (xHoldsSP_, aEqualsX_) with unified state.

enum RegId : uint8_t {
    REG_A  = 0,
    REG_X  = 1,
    REG_Y  = 2,
    REG_Z  = 3,
    REG_SP = 4,
    REG_COUNT = 5
};

struct ValueInfo {
    enum Kind : uint8_t {
        UNKNOWN,        // no information
        CONSTANT,       // known exact value
        SAME_AS_REG,    // mirrors another register (e.g., A==X after tax)
    };
    Kind kind = UNKNOWN;
    int64_t constVal = 0;       // for CONSTANT
    uint8_t mirrorReg = 0;      // for SAME_AS_REG (RegId)

    bool isUnknown() const { return kind == UNKNOWN; }
    bool isConst() const { return kind == CONSTANT; }
    bool isConst(int64_t v) const { return kind == CONSTANT && constVal == v; }
    bool isZero() const { return isConst(0); }
    bool isSameAs(RegId r) const { return kind == SAME_AS_REG && mirrorReg == r; }

    static ValueInfo unknown() { return {}; }
    static ValueInfo constant(int64_t v) { return {CONSTANT, v, 0}; }
    static ValueInfo sameAs(RegId r) { return {SAME_AS_REG, 0, (uint8_t)r}; }
};

struct FlagState {
    enum FlagVal : uint8_t { F_UNKNOWN, F_SET, F_CLEAR };
    FlagVal n = F_UNKNOWN;   // Negative
    FlagVal z = F_UNKNOWN;   // Zero
    FlagVal c = F_UNKNOWN;   // Carry
    FlagVal v = F_UNKNOWN;   // Overflow
    // Which register last set N/Z flags? -1 = unknown.
    // Enables compare elimination when flags already reflect a register.
    int8_t nzSourceReg = -1;

    void invalidate() { n = z = c = v = F_UNKNOWN; nzSourceReg = -1; }
    void setNZ(RegId src) { n = F_UNKNOWN; z = F_UNKNOWN; nzSourceReg = (int8_t)src; }
    void setCarry(bool set) { c = set ? F_SET : F_CLEAR; }
};

struct MachineState {
    ValueInfo reg[REG_COUNT];  // A, X, Y, Z, SP
    FlagState flags;

    // --- Register queries ---

    // True when X is known to hold the stack pointer value (tsx caching).
    bool xHoldsSP() const { return reg[REG_X].isSameAs(REG_SP); }

    // True when registers a and b are known to hold the same value.
    bool regsEqual(RegId a, RegId b) const {
        if (a == b) return true;
        const auto& va = reg[a];
        const auto& vb = reg[b];
        // Both constant with same value
        if (va.isConst() && vb.isConst() && va.constVal == vb.constVal) return true;
        // One mirrors the other
        if (va.isSameAs(b) || vb.isSameAs(a)) return true;
        // Both mirror the same third register
        if (va.kind == ValueInfo::SAME_AS_REG && vb.kind == ValueInfo::SAME_AS_REG
            && va.mirrorReg == vb.mirrorReg) return true;
        return false;
    }

    // --- State updates ---

    // Set a register to a known constant.
    void setConst(RegId r, int64_t val) {
        invalidateMirrorsOf(r);
        reg[r] = ValueInfo::constant(val);
    }

    // Set a register to mirror another (e.g., tax → X = SAME_AS(A)).
    // If the source is a known constant, propagate it instead.
    void setTransfer(RegId dst, RegId src) {
        invalidateMirrorsOf(dst);
        if (reg[src].isConst()) {
            reg[dst] = reg[src]; // propagate constant
        } else {
            reg[dst] = ValueInfo::sameAs(src);
        }
        // Transfer instructions set N/Z from the transferred value
        flags.setNZ(dst);
    }

    // Mark a register as unknown (e.g., after an instruction clobbers it).
    void invalidateReg(RegId r) {
        invalidateMirrorsOf(r);
        reg[r] = ValueInfo::unknown();
        if (flags.nzSourceReg == (int8_t)r) flags.nzSourceReg = -1;
    }

    // Mark everything unknown (e.g., at labels, branch targets, JSR).
    void invalidateAll() {
        for (int i = 0; i < REG_COUNT; i++)
            reg[i] = ValueInfo::unknown();
        flags.invalidate();
    }

    // --- Stack pointer tracking ---

    // SP was modified (push/pull). Invalidate SP and anything mirroring it.
    void spModified() {
        invalidateMirrorsOf(REG_SP);
        reg[REG_SP] = ValueInfo::unknown();
    }

private:
    // When register r is about to change, clear any other register
    // that has SAME_AS_REG pointing to r.
    void invalidateMirrorsOf(RegId r) {
        for (int i = 0; i < REG_COUNT; i++) {
            if (i != (int)r && reg[i].isSameAs(r)) {
                reg[i] = ValueInfo::unknown();
            }
        }
    }
};
