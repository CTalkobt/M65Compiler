#include "VRegAllocator.hpp"
#include <algorithm>

// ============================================================================
// Flatten function instructions into linear sequence
// ============================================================================

void VRegAllocator::flatten(const ir::Function& fn) {
    flatInsts_.clear();
    for (int bi = 0; bi < (int)fn.blocks.size(); bi++) {
        for (int ii = 0; ii < (int)fn.blocks[bi].insts.size(); ii++) {
            flatInsts_.push_back({&fn.blocks[bi].insts[ii], bi, ii});
        }
    }
}

// ============================================================================
// Liveness analysis
// ============================================================================

static void recordUse(std::map<uint32_t, VRegAllocator::LiveRange>& map,
                      uint32_t vregId, int instIdx, ir::Type type) {
    auto& lr = map[vregId];
    lr.vregId = vregId;
    lr.type = type;
    if (lr.firstDef < 0) lr.firstDef = instIdx;
    lr.lastUse = instIdx;
}

static void recordDef(std::map<uint32_t, VRegAllocator::LiveRange>& map,
                      uint32_t vregId, int instIdx, ir::Type type) {
    auto& lr = map[vregId];
    lr.vregId = vregId;
    lr.type = type;
    if (lr.firstDef < 0) lr.firstDef = instIdx;
    if (lr.lastUse < instIdx) lr.lastUse = instIdx;
}

void VRegAllocator::computeLiveRanges(const ir::Function& fn) {
    std::map<uint32_t, LiveRange> map;

    // Parameters are defined at instruction -1 (before first inst)
    for (uint32_t i = 0; i < (uint32_t)fn.paramTypes.size(); i++) {
        recordDef(map, i, 0, fn.paramTypes[i]);
    }

    // Local variables are also effectively "defined" from start
    for (const auto& [name, vregId] : fn.localNames) {
        ir::Type t = ir::Type::I16;
        if (fn.vregTypes.count(vregId)) {
            t = fn.vregTypes.at(vregId);
        } else if (fn.vregSizes.count(vregId)) {
            int sz = fn.vregSizes.at(vregId);
            if (sz == 1) t = ir::Type::I8;
            else if (sz == 4) t = ir::Type::I32;
        }
        recordDef(map, vregId, 0, t);
    }

    for (int idx = 0; idx < (int)flatInsts_.size(); idx++) {
        const ir::Inst& inst = *flatInsts_[idx].inst;

        // Record uses (src1, src2, args)
        if (inst.src1.isVreg()) recordUse(map, inst.src1.vregId, idx, inst.src1.type);
        if (inst.src2.isVreg()) recordUse(map, inst.src2.vregId, idx, inst.src2.type);
        for (const auto& arg : inst.args) {
            if (arg.isVreg()) recordUse(map, arg.vregId, idx, arg.type);
        }
        for (const auto& phi : inst.phiIncoming) {
            if (phi.first.isVreg()) recordUse(map, phi.first.vregId, idx, phi.first.type);
        }

        // Record definition (dest)
        if (inst.dest.isVreg()) {
            ir::Type t = inst.resultType != ir::Type::VOID ? inst.resultType : ir::Type::I16;
            // Prefer type from vregTypes if available
            if (fn.vregTypes.count(inst.dest.vregId)) t = fn.vregTypes.at(inst.dest.vregId);
            recordDef(map, inst.dest.vregId, idx, t);
        }
    }

    ranges_.clear();
    for (auto& [id, lr] : map) {
        ranges_.push_back(lr);
    }
    // Sort by firstDef for linear scan
    std::sort(ranges_.begin(), ranges_.end(),
              [](const LiveRange& a, const LiveRange& b) { return a.firstDef < b.firstDef; });
}

// ============================================================================
// Location assignment
// ============================================================================

int VRegAllocator::allocZpSlot(ir::Type type) {
    int size = ir::typeSize(type);
    if (size < 2) size = 2;
    // Find consecutive free ZP bytes
    for (int i = 0; i + size <= (int)zpUsed_.size(); i += 2) {
        bool free = true;
        for (int j = 0; j < size; j++) {
            if (zpUsed_[i + j]) { free = false; break; }
        }
        if (free) {
            for (int j = 0; j < size; j++) zpUsed_[i + j] = true;
            return zpStart_ + i;
        }
    }
    return -1; // no ZP available
}

void VRegAllocator::freeZpSlot(int zpAddr, ir::Type type) {
    int size = ir::typeSize(type);
    if (size < 2) size = 2;
    int base = zpAddr - zpStart_;
    for (int j = 0; j < size && base + j < (int)zpUsed_.size(); j++) {
        zpUsed_[base + j] = false;
    }
}

int VRegAllocator::allocFrameSlot(ir::Type type, int overrideSize) {
    int size = (overrideSize > 0) ? overrideSize : ir::typeSize(type);
    if (size < 2) size = 2;
    int offset = frameSize_;
    frameSize_ += size;
    return offset;
}

void VRegAllocator::assignLocations(const ir::Function& fn) {
    allocs_.clear();
    frameSize_ = 0;

    // Initialize ZP pool
    int zpPoolBytes = zpSlots_ * 2;
    zpUsed_.assign(zpPoolBytes, false);

    // Initialize A:X tracking
    axState_.assign(flatInsts_.size() + 1, -1);

    // Identify call instruction indices (A:X is clobbered after calls)
    std::set<int> callIndices;
    for (int idx = 0; idx < (int)flatInsts_.size(); idx++) {
        ir::Op op = flatInsts_[idx].inst->op;
        if (op == ir::Op::CALL || op == ir::Op::CALL_VOID || op == ir::Op::CALL_INDIRECT) {
            callIndices.insert(idx);
        }
    }

    // Track active ZP allocations for freeing
    // Map vregId → zpAddr for ZP-allocated vRegs
    std::map<uint32_t, int> zpAllocMap;

    uint32_t paramCount = (uint32_t)fn.paramTypes.size();
    int axOccupiedUntil = -1; // instruction index until which A:X is occupied

    for (auto& lr : ranges_) {
        int span = lr.lastUse - lr.firstDef;

        // Check if any call occurs within the live range
        bool crossesCall = false;
        for (int ci : callIndices) {
            if (ci > lr.firstDef && ci <= lr.lastUse) {
                crossesCall = true;
                break;
            }
        }

        // Parameters always go to ZP or frame (they arrive on stack, not in AX)
        bool isParam = (lr.vregId < paramCount);

        // Array vRegs must go to frame (too large for ZP)
        bool isArray = fn.vregSizes.count(lr.vregId) > 0;
        bool isMemory = fn.memoryVregs.count(lr.vregId) > 0;
        if (isArray || isMemory) {
            int size = fn.vregSizes.count(lr.vregId) ? fn.vregSizes.at(lr.vregId) : ir::typeSize(lr.type);
            allocs_[lr.vregId] = {IN_FRAME, allocFrameSlot(lr.type, size), lr.type};
            continue;
        }

        // Disable IN_AX allocation: src2MemOperand for FRAME vRegs uses
        // ldax.fp which clobbers AX, destroying IN_AX values between
        // their definition and use. Use ZP or FRAME only.
        bool canUseAX = false;

        if (canUseAX && span <= 1) {
            // Short-lived, no conflict — keep in A:X
            allocs_[lr.vregId] = {IN_AX, 0, lr.type};
            axOccupiedUntil = lr.lastUse;
            for (int i = lr.firstDef; i <= lr.lastUse && i < (int)axState_.size(); i++) {
                axState_[i] = (int)lr.vregId;
            }
        } else if (!crossesCall) {
            // Try ZP for medium-lived vRegs that don't cross calls
            int zpAddr = allocZpSlot(lr.type);
            if (zpAddr >= 0) {
                allocs_[lr.vregId] = {IN_ZP, zpAddr, lr.type};
                zpAllocMap[lr.vregId] = zpAddr;
            } else {
                allocs_[lr.vregId] = {IN_FRAME, allocFrameSlot(lr.type), lr.type};
            }
        } else {
            // Crosses a call — ZP is clobbered by callees, must use frame
            {
                allocs_[lr.vregId] = {IN_FRAME, allocFrameSlot(lr.type), lr.type};
            }
        }
    }

    // Free ZP slots after last use (for future functions — not critical for single-function)
    // In practice, ZP pool is reset per function.
}

// ============================================================================
// Public API
// ============================================================================

void VRegAllocator::analyze(const ir::Function& fn, uint8_t zpStart, int zpSlots) {
    zpStart_ = zpStart;
    zpSlots_ = zpSlots;
    flatten(fn);
    computeLiveRanges(fn);
    assignLocations(fn);
}

VRegAllocator::Allocation VRegAllocator::getAlloc(uint32_t vregId) const {
    auto it = allocs_.find(vregId);
    if (it != allocs_.end()) return it->second;
    return {IN_FRAME, 0, ir::Type::I16};
}

bool VRegAllocator::isInAX(uint32_t vregId, int instIndex) const {
    if (instIndex < 0 || instIndex >= (int)axState_.size()) return false;
    return axState_[instIndex] == (int)vregId;
}

int VRegAllocator::axContents(int instIndex) const {
    if (instIndex < 0 || instIndex >= (int)axState_.size()) return -1;
    return axState_[instIndex];
}
