#pragma once
#include <cstdint>
#include <array>
#include <string>
#include <sstream>

// Register & Memory Value Tracking Framework — Phase 1+2+3
// Tracks what is known about CPU register values, memory contents,
// and CPU flags during code emission.
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
        NONZERO,        // known to be != 0 (but exact value unknown)
        RANGE,          // bounded range [rangeLo..rangeHi]
        RELOC_CONST,    // relocatable symbol byte (lo or hi), resolved at link time
    };
    // Byte selector for RELOC_CONST
    enum RelocByte : uint8_t { RELOC_LO = 0, RELOC_HI = 1 };

    Kind kind = UNKNOWN;
    int64_t constVal = 0;       // for CONSTANT (also assembly-time value for RELOC_CONST)
    uint8_t mirrorReg = 0;      // for SAME_AS_REG (RegId)
    uint16_t mirrorAddr = 0;    // for SAME_AS_MEM
    int64_t rangeLo = 0;        // for RANGE
    int64_t rangeHi = 0;        // for RANGE
    std::string relocSymbol;    // for RELOC_CONST: symbol name
    RelocByte relocByte = RELOC_LO; // for RELOC_CONST: lo or hi byte

    bool isUnknown() const { return kind == UNKNOWN; }
    bool isConst() const { return kind == CONSTANT; }
    bool isConst(int64_t v) const { return kind == CONSTANT && constVal == v; }
    bool isZero() const { return isConst(0); }
    bool isSameAs(RegId r) const { return kind == SAME_AS_REG && mirrorReg == r; }
    bool isSameAsMem(uint16_t addr) const { return kind == SAME_AS_MEM && mirrorAddr == addr; }
    bool isReloc() const { return kind == RELOC_CONST; }

    // True if the value is known to be non-zero.
    bool isNonZero() const {
        if (kind == NONZERO) return true;
        if (kind == CONSTANT) return constVal != 0;
        if (kind == RANGE) return (rangeLo > 0) || (rangeHi < 0);
        return false;
    }

    // True if the value is known to be within [lo..hi].
    bool isInRange(int64_t lo, int64_t hi) const {
        if (kind == CONSTANT) return constVal >= lo && constVal <= hi;
        if (kind == RANGE) return rangeLo >= lo && rangeHi <= hi;
        return false;
    }

    // True if the value is a known constant OR a relocatable constant
    // (both can be used for immediate-mode forwarding).
    bool isConstOrReloc() const { return kind == CONSTANT || kind == RELOC_CONST; }

    static ValueInfo unknown() { return {}; }
    static ValueInfo constant(int64_t v) { ValueInfo vi; vi.kind = CONSTANT; vi.constVal = v; return vi; }
    static ValueInfo sameAs(RegId r) { ValueInfo vi; vi.kind = SAME_AS_REG; vi.mirrorReg = (uint8_t)r; return vi; }
    static ValueInfo sameAsMem(uint16_t addr) { ValueInfo vi; vi.kind = SAME_AS_MEM; vi.mirrorAddr = addr; return vi; }
    static ValueInfo nonZero() { ValueInfo vi; vi.kind = NONZERO; return vi; }
    static ValueInfo range(int64_t lo, int64_t hi) { ValueInfo vi; vi.kind = RANGE; vi.rangeLo = lo; vi.rangeHi = hi; return vi; }
    static ValueInfo reloc(const std::string& sym, RelocByte byte, int64_t asmVal = 0) {
        ValueInfo vi; vi.kind = RELOC_CONST; vi.relocSymbol = sym;
        vi.relocByte = byte; vi.constVal = asmVal; return vi;
    }
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

    // Record that N/Z flags were set from a value with known properties.
    // If the value is a known constant, derive exact N/Z.
    // If NONZERO, Z is known CLEAR.
    void setNZFromValue(const ValueInfo& val, int8_t srcReg) {
        nzSourceReg = srcReg;
        if (val.isConst()) {
            uint8_t byte = (uint8_t)(val.constVal & 0xFF);
            z = (byte == 0) ? F_SET : F_CLEAR;
            n = (byte & 0x80) ? F_SET : F_CLEAR;
        } else if (val.kind == ValueInfo::RANGE) {
            // Derive what we can from the range
            uint8_t lo = (uint8_t)(val.rangeLo & 0xFF);
            uint8_t hi = (uint8_t)(val.rangeHi & 0xFF);
            z = (val.rangeLo > 0 || val.rangeHi < 0) ? F_CLEAR : F_UNKNOWN;
            // If entire range has bit 7 set, N is SET; if none do, N is CLEAR
            n = (lo >= 0x80 && hi >= 0x80) ? F_SET :
                (hi < 0x80) ? F_CLEAR : F_UNKNOWN;
        } else if (val.isNonZero()) {
            z = F_CLEAR;
            n = F_UNKNOWN;
        } else {
            z = F_UNKNOWN;
            n = F_UNKNOWN;
        }
    }

    // Simple setNZ when only the register ID is known (no value info).
    void setNZ(RegId src) { n = F_UNKNOWN; z = F_UNKNOWN; nzSourceReg = (int8_t)src; }

    void setCarry(bool set) { c = set ? F_SET : F_CLEAR; }

    // True if N/Z flags currently reflect the given register's value.
    bool flagsReflect(RegId r) const { return nzSourceReg == (int8_t)r; }

    // True if we know the Z flag state (either SET or CLEAR).
    bool zKnown() const { return z != F_UNKNOWN; }
    bool nKnown() const { return n != F_UNKNOWN; }
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
        flags.setNZFromValue(reg[r], (int8_t)r);
    }

    // Set a register to a relocatable constant (symbol lo/hi byte).
    void setReloc(RegId r, const std::string& sym, ValueInfo::RelocByte byte, int64_t asmVal = 0) {
        invalidateMirrorsOf(r);
        reg[r] = ValueInfo::reloc(sym, byte, asmVal);
        flags.setNZFromValue(reg[r], (int8_t)r);
    }

    // Set a register to mirror another (e.g., tax → X = SAME_AS(A)).
    // If the source is a known constant or reloc, propagate it instead.
    void setTransfer(RegId dst, RegId src) {
        invalidateMirrorsOf(dst);
        if (reg[src].isConst() || reg[src].isReloc()) {
            reg[dst] = reg[src]; // propagate constant or reloc
        } else {
            reg[dst] = ValueInfo::sameAs(src);
        }
        // Transfer instructions set N/Z from the transferred value
        flags.setNZFromValue(reg[dst], (int8_t)dst);
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
        flags.setNZFromValue(reg[r], (int8_t)r);
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
        pushDepth_ = 0;
    }

    // Selectively invalidate only the specified registers (Phase 2: fine-grained clobber tracking).
    // clobberMask is a bitmask of registers (1 << REG_A, 1 << REG_X, etc.)
    void invalidateSelective(int clobberMask) {
        if (clobberMask & (1 << REG_A)) invalidateReg(REG_A);
        if (clobberMask & (1 << REG_X)) invalidateReg(REG_X);
        if (clobberMask & (1 << REG_Y)) invalidateReg(REG_Y);
        if (clobberMask & (1 << REG_Z)) invalidateReg(REG_Z);
        // Note: only invalidate flags if all registers are clobbered (conservative)
        // Phase 2: could refine this with separate flag clobber masks later
    }

    // --- Memory state updates ---

    // Record a store to ZP: the memory location now holds what the register holds.
    void storeZP(uint8_t zpAddr, RegId srcReg) {
        invalidateMemMirrorsOf(zpAddr);
        if (reg[srcReg].isConst() || reg[srcReg].isReloc()) {
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

    // --- Register value updates (non-standard) ---

    // Set a register to NONZERO (known != 0 but exact value unknown).
    void setNonZero(RegId r) {
        invalidateMirrorsOf(r);
        reg[r] = ValueInfo::nonZero();
        flags.setNZFromValue(reg[r], (int8_t)r);
    }

    // Set a register to a known range [lo..hi].
    void setRange(RegId r, int64_t lo, int64_t hi) {
        invalidateMirrorsOf(r);
        reg[r] = ValueInfo::range(lo, hi);
        flags.setNZFromValue(reg[r], (int8_t)r);
    }

    // Public mirror invalidation for callers that update reg[] directly.
    void invalidateMirrorsOfPub(RegId r) { invalidateMirrorsOf(r); }

    // --- Hardware stack push/pull tracking ---
    // Track a small push/pull stack so pha/pla (and cross-register pha/plx)
    // can preserve register knowledge through the stack.
    static constexpr int PUSH_STACK_SIZE = 8;
    ValueInfo pushStack_[PUSH_STACK_SIZE];
    int pushDepth_ = 0;

    // Push a register's value onto the tracking stack.
    void pushReg(RegId r) {
        if (pushDepth_ < PUSH_STACK_SIZE) {
            pushStack_[pushDepth_++] = reg[r];
        } else {
            pushDepth_++; // overflow — track depth but lose value
        }
        invalidateMirrorsOf(REG_SP);
        reg[REG_SP] = ValueInfo::unknown();
    }

    // Pull a value from the tracking stack into a register.
    void pullReg(RegId r) {
        invalidateMirrorsOf(r);
        if (pushDepth_ > 0 && pushDepth_ <= PUSH_STACK_SIZE) {
            reg[r] = pushStack_[--pushDepth_];
            // N/Z flags set from the pulled value
            flags.setNZFromValue(reg[r], (int8_t)r);
        } else {
            if (pushDepth_ > 0) pushDepth_--;
            reg[r] = ValueInfo::unknown();
            flags.setNZ(r);
        }
        invalidateMirrorsOf(REG_SP);
        reg[REG_SP] = ValueInfo::unknown();
    }

    // SP was modified by something other than push/pull (e.g., txs).
    // Invalidate SP and reset push tracking.
    void spModified() {
        invalidateMirrorsOf(REG_SP);
        reg[REG_SP] = ValueInfo::unknown();
        pushDepth_ = 0; // can't trust stack contents
    }

    // Compact one-line dump of register and flag state for tracing
    std::string dumpLine() const {
        static const char* regNames[] = {"A", "X", "Y", "Z", "SP"};
        std::ostringstream os;
        os << "  regs: ";
        for (int i = 0; i < REG_COUNT; i++) {
            const auto& v = reg[i];
            os << regNames[i] << "=";
            switch (v.kind) {
                case ValueInfo::UNKNOWN:     os << "?"; break;
                case ValueInfo::CONSTANT:    os << "$" << std::hex << (v.constVal & 0xFF); break;
                case ValueInfo::SAME_AS_REG: os << "=" << regNames[v.mirrorReg]; break;
                case ValueInfo::SAME_AS_MEM: os << "[$" << std::hex << v.mirrorAddr << "]"; break;
                case ValueInfo::NONZERO:     os << "!0"; break;
                case ValueInfo::RANGE:       os << std::dec << v.rangeLo << ".." << v.rangeHi; break;
                case ValueInfo::RELOC_CONST: os << "<" << v.relocSymbol << (v.relocByte ? ">hi" : ">lo"); break;
            }
            if (i < REG_COUNT - 1) os << " ";
        }
        os << std::dec << "  flags: ";
        auto fv = [](FlagState::FlagVal f) { return f == FlagState::F_SET ? "1" : f == FlagState::F_CLEAR ? "0" : "?"; };
        os << "N=" << fv(flags.n) << " Z=" << fv(flags.z)
           << " C=" << fv(flags.c) << " V=" << fv(flags.v);
        return os.str();
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
