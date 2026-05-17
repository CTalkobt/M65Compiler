#pragma once
#include "IR.hpp"
#include <map>
#include <set>
#include <vector>
#include <cstdint>

class VRegAllocator {
public:
    enum Location { IN_AX, IN_ZP, IN_FRAME };

    struct Allocation {
        Location loc = IN_FRAME;
        int offset = 0;       // ZP address (for IN_ZP) or frame offset (for IN_FRAME)
        ir::Type type = ir::Type::I16;
    };

    struct LiveRange {
        uint32_t vregId;
        int firstDef = -1;    // instruction index of definition
        int lastUse = -1;     // instruction index of last use
        ir::Type type = ir::Type::I16;
    };

    // Analyze a function: compute live ranges and assign allocations
    void analyze(const ir::Function& fn, uint8_t zpStart = 0x20, int zpSlots = 64);

    // Get the allocation for a vReg
    Allocation getAlloc(uint32_t vregId) const;

    // Is the given vReg currently in A:X at this instruction point?
    // (i.e., was it just defined and hasn't been evicted yet)
    bool isInAX(uint32_t vregId, int instIndex) const;

    // Get the vReg currently in A:X at a given instruction, or -1
    int axContents(int instIndex) const;

    // Total frame bytes needed (only for frame-allocated vRegs)
    int frameSize() const { return frameSize_; }

    // Live ranges (for debugging/inspection)
    const std::vector<LiveRange>& liveRanges() const { return ranges_; }

private:
    std::vector<LiveRange> ranges_;
    std::map<uint32_t, Allocation> allocs_;

    // A:X state: which vReg is in A:X at each instruction index
    // -1 means A:X is empty/unknown
    std::vector<int> axState_;

    // ZP pool
    uint8_t zpStart_ = 0x20;
    int zpSlots_ = 8;
    std::vector<bool> zpUsed_;

    int frameSize_ = 0;

    void computeLiveRanges(const ir::Function& fn);
    void assignLocations(const ir::Function& fn);

    int allocZpSlot(ir::Type type);
    void freeZpSlot(int zpAddr, ir::Type type);
    int allocFrameSlot(ir::Type type, int overrideSize = 0);

    // Flatten all instructions in a function to a linear index
    struct FlatInst {
        const ir::Inst* inst;
        int blockIdx;
        int instIdx;
    };
    std::vector<FlatInst> flatInsts_;
    void flatten(const ir::Function& fn);
};
