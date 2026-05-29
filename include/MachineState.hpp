#pragma once
#include <cstdint>
#include <array>

// Register & Memory Value Tracking Framework — Phase 1+2
// Tracks what is known about CPU register values and memory contents
// during code emission.
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
        SAME_AS_MEM,    // mirrors a memory location (e.g., A == [$20] after lda $20)
    };
    Kind kind = UNKNOWN;
    int64_t constVal = 0;       // for CONSTANT
    uint8_t mirrorReg = 0;      // for SAME_AS_REG (RegId)
    uint16_t mirrorAddr = 0;    // for SAME_AS_MEM

    bool isUnknown() const { return kind == UNKNOWN; }
    bool isConst() const { return kind == CONSTANT; }
    bool isConst(int64_t v) const { return kind == CONSTANT && constVal == v; }
    bool isZero() const { return isConst(0); }
    bool isSameAs(RegId r) const { return kind == SAME_AS_REG && mirrorReg == r; }
    bool isSameAsMem(uint16_t addr) const { return kind == SAME_AS_MEM && mirrorAddr == addr; }

    static ValueInfo unknown() { return {}; }
    static ValueInfo constant(int64_t v) { return {CONSTANT, v, 0, 0}; }
    static ValueInfo sameAs(RegId r) { return {SAME_AS_REG, 0, (uint8_t)r, 0}; }
    static ValueInfo sameAsMem(uint16_t addr) { return {SAME_AS_MEM, 0, 0, addr}; }
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

// ============================================================================
// Three-tier memory tracking
// ============================================================================

// Tier 3: General memory ring buffer entry
struct MemEntry {
    uint16_t addr = 0;     // absolute address (0 = unused)
    ValueInfo info;         // what's known about this location
    uint32_t age = 0;       // access counter for LRU eviction
    bool valid = false;     // true if this entry is in use
};

static constexpr int MEM_CACHE_SIZE = 512;

struct MachineState {
    ValueInfo reg[REG_COUNT];  // A, X, Y, Z, SP
    FlagState flags;

    // --- Three-tier memory tracking ---
    // Tier 1: Zero page — full coverage ($00–$FF)
    ValueInfo zpMem[256];
    // Tier 2: Stack — full coverage (indexed by stack offset)
    ValueInfo stackMem[256];
    // Tier 3: General memory — ring buffer for absolute addresses
    std::array<MemEntry, MEM_CACHE_SIZE> memCache;
    uint32_t memAge_ = 0;  // monotonic access counter for LRU

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

    // --- Memory queries ---

    // Get what's known about a ZP location.
    const ValueInfo& getZP(uint8_t addr) const { return zpMem[addr]; }

    // Get what's known about a stack offset.
    const ValueInfo& getStack(uint8_t offset) const { return stackMem[offset]; }

    // Get what's known about an absolute address (from ring buffer).
    // Returns UNKNOWN if not tracked.
    ValueInfo getAbsMem(uint16_t addr) const {
        for (const auto& e : memCache) {
            if (e.valid && e.addr == addr) return e.info;
        }
        return ValueInfo::unknown();
    }

    // --- Register state updates ---

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

    // Set a register from a load from memory. If the memory value is known,
    // propagate it; otherwise mark as SAME_AS_MEM.
    void setRegFromZP(RegId r, uint8_t zpAddr) {
        invalidateMirrorsOf(r);
        const auto& memVal = zpMem[zpAddr];
        if (memVal.isConst()) {
            reg[r] = memVal;  // propagate known constant
        } else {
            reg[r] = ValueInfo::sameAsMem(zpAddr);
        }
        flags.setNZ(r);
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
        invalidateAllMem();
    }

    // --- Memory state updates ---

    // Record a store to ZP: the memory location now holds what the register holds.
    void storeZP(uint8_t zpAddr, RegId srcReg) {
        invalidateMemMirrorsOf(zpAddr);
        if (reg[srcReg].isConst()) {
            zpMem[zpAddr] = reg[srcReg];  // propagate constant
        } else {
            zpMem[zpAddr] = ValueInfo::sameAs(srcReg);
        }
    }

    // Record a store of a known constant to ZP (e.g., stz when Z==0).
    void storeZPConst(uint8_t zpAddr, int64_t val) {
        invalidateMemMirrorsOf(zpAddr);
        zpMem[zpAddr] = ValueInfo::constant(val);
    }

    // Invalidate a ZP location (e.g., after inc/dec/asl/lsr/rol/ror modifying memory).
    void invalidateZP(uint8_t zpAddr) {
        invalidateMemMirrorsOf(zpAddr);
        zpMem[zpAddr] = ValueInfo::unknown();
    }

    // Record a store to a stack offset.
    void storeStack(uint8_t offset, RegId srcReg) {
        if (reg[srcReg].isConst()) {
            stackMem[offset] = reg[srcReg];
        } else {
            stackMem[offset] = ValueInfo::sameAs(srcReg);
        }
    }

    // Invalidate a stack offset.
    void invalidateStack(uint8_t offset) {
        stackMem[offset] = ValueInfo::unknown();
    }

    // Invalidate all stack state (e.g., after JSR which may modify stack).
    void invalidateAllStack() {
        for (int i = 0; i < 256; i++)
            stackMem[i] = ValueInfo::unknown();
    }

    // Record a store to an absolute address (ring buffer).
    void storeAbs(uint16_t addr, RegId srcReg) {
        auto* entry = findOrAllocMemEntry(addr);
        if (reg[srcReg].isConst()) {
            entry->info = reg[srcReg];
        } else {
            entry->info = ValueInfo::sameAs(srcReg);
        }
    }

    // Invalidate an absolute address in the ring buffer.
    void invalidateAbs(uint16_t addr) {
        for (auto& e : memCache) {
            if (e.valid && e.addr == addr) {
                e.valid = false;
                break;
            }
        }
    }

    // Invalidate all memory state.
    void invalidateAllMem() {
        for (int i = 0; i < 256; i++) {
            zpMem[i] = ValueInfo::unknown();
            stackMem[i] = ValueInfo::unknown();
        }
        for (auto& e : memCache) e.valid = false;
        memAge_ = 0;
    }

    // --- Stack pointer tracking ---

    // SP was modified (push/pull). Invalidate SP and anything mirroring it.
    void spModified() {
        invalidateMirrorsOf(REG_SP);
        reg[REG_SP] = ValueInfo::unknown();
    }

private:
    // When register r is about to change, clear any other register
    // that has SAME_AS_REG pointing to r. Also clear memory entries
    // that mirror this register.
    void invalidateMirrorsOf(RegId r) {
        for (int i = 0; i < REG_COUNT; i++) {
            if (i != (int)r && reg[i].isSameAs(r)) {
                reg[i] = ValueInfo::unknown();
            }
        }
        // Memory entries mirroring this register become unknown
        for (int i = 0; i < 256; i++) {
            if (zpMem[i].isSameAs(r)) zpMem[i] = ValueInfo::unknown();
            if (stackMem[i].isSameAs(r)) stackMem[i] = ValueInfo::unknown();
        }
        for (auto& e : memCache) {
            if (e.valid && e.info.isSameAs(r)) e.info = ValueInfo::unknown();
        }
    }

    // When a memory location is about to change, clear any register
    // that has SAME_AS_MEM pointing to it.
    void invalidateMemMirrorsOf(uint16_t addr) {
        for (int i = 0; i < REG_COUNT; i++) {
            if (reg[i].isSameAsMem(addr)) {
                reg[i] = ValueInfo::unknown();
            }
        }
    }

    // Find or allocate a ring buffer entry for an absolute address.
    MemEntry* findOrAllocMemEntry(uint16_t addr) {
        // Look for existing entry
        for (auto& e : memCache) {
            if (e.valid && e.addr == addr) {
                e.age = ++memAge_;
                return &e;
            }
        }
        // Find an unused slot
        for (auto& e : memCache) {
            if (!e.valid) {
                e.addr = addr;
                e.valid = true;
                e.age = ++memAge_;
                return &e;
            }
        }
        // Evict LRU entry
        MemEntry* oldest = &memCache[0];
        for (auto& e : memCache) {
            if (e.age < oldest->age) oldest = &e;
        }
        oldest->addr = addr;
        oldest->valid = true;
        oldest->info = ValueInfo::unknown();
        oldest->age = ++memAge_;
        return oldest;
    }
};
