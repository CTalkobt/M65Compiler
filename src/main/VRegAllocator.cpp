#include "VRegAllocator.hpp"
#include <algorithm>
#include <iostream>

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
    // Sort by firstDef for linear scan, then by vregId for deterministic ordering
    std::sort(ranges_.begin(), ranges_.end(),
              [](const LiveRange& a, const LiveRange& b) {
                  if (a.firstDef != b.firstDef) return a.firstDef < b.firstDef;
                  return a.vregId < b.vregId;
              });
}

// ============================================================================
// Loop-aware liveness extension
// ============================================================================

void VRegAllocator::extendLiveRangesAcrossLoops(const ir::Function& fn) {
    // The linear scan computes live ranges as [firstDef, lastUse] over the
    // flattened instruction sequence. This doesn't account for loops: a vreg
    // defined before a loop and used inside the loop body needs to stay live
    // for the entire loop, not just until the first (linear) use.
    //
    // Fix: detect loops via back-edges, compute the flattened index range of
    // each loop, and extend any live range that overlaps the loop to cover
    // the entire loop body.

    if (flatInsts_.empty()) return;

    // Build block → flattened index range mapping
    // blockStart[b] = first flattened index in block b
    // blockEnd[b]   = last flattened index in block b (inclusive)
    std::map<int, int> blockStart, blockEnd;
    for (int i = 0; i < (int)flatInsts_.size(); i++) {
        int bi = flatInsts_[i].blockIdx;
        if (!blockStart.count(bi)) blockStart[bi] = i;
        blockEnd[bi] = i;
    }

    // Build block label → block index mapping
    std::map<std::string, int> labelToBlock;
    for (int bi = 0; bi < (int)fn.blocks.size(); bi++) {
        labelToBlock[fn.blocks[bi].label] = bi;
    }

    // Find back-edges and compute loop ranges.
    // A back-edge is a branch from block B to block H where H <= B in block order.
    // The loop range in flattened indices is [blockStart[H], blockEnd[B]].
    struct LoopRange { int start; int end; };
    std::vector<LoopRange> loops;

    for (int bi = 0; bi < (int)fn.blocks.size(); bi++) {
        for (const auto& inst : fn.blocks[bi].insts) {
            // Collect branch targets
            std::vector<std::string> targets;
            if (inst.op == ir::Op::BR && !inst.src1.name.empty())
                targets.push_back(inst.src1.name);
            if (inst.op == ir::Op::BR_COND) {
                if (!inst.dest.name.empty()) targets.push_back(inst.dest.name);
                if (!inst.src2.name.empty()) targets.push_back(inst.src2.name);
            }
            for (const auto& t : targets) {
                auto it = labelToBlock.find(t);
                if (it != labelToBlock.end() && it->second <= bi) {
                    // Back-edge: bi → it->second
                    int hdr = it->second;
                    if (blockStart.count(hdr) && blockEnd.count(bi)) {
                        loops.push_back({blockStart[hdr], blockEnd[bi]});
                    }
                }
            }
        }
    }

    if (loops.empty()) return;

    // Extend live ranges: if a vreg's live range overlaps with a loop range,
    // extend lastUse to at least the loop's end.
    for (auto& lr : ranges_) {
        for (const auto& loop : loops) {
            // A live range overlaps a loop if it's defined before/during the loop
            // AND used during/after the loop start
            if (lr.firstDef <= loop.end && lr.lastUse >= loop.start) {
                if (lr.lastUse < loop.end) {
                    lr.lastUse = loop.end;
                }
            }
        }
    }
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

    // Try to reuse a freed frame slot of matching size
    for (auto it = freeFrameSlots_.begin(); it != freeFrameSlots_.end(); ++it) {
        if (it->second == size) {
            int offset = it->first;
            freeFrameSlots_.erase(it);
            return offset;
        }
    }

    int offset = frameSize_;
    frameSize_ += size;
    return offset;
}

void VRegAllocator::freeFrameSlot(int offset, int size) {
    freeFrameSlots_.push_back({offset, size});
}

void VRegAllocator::assignLocations(const ir::Function& fn) {
    allocs_.clear();
    frameSize_ = 0;
    freeFrameSlots_.clear();

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

    // Track active frame allocations for reuse
    // Map vregId → {frameOffset, slotSize}
    struct FrameSlotInfo { int offset; int size; };
    std::map<uint32_t, FrameSlotInfo> frameAllocMap;

    uint32_t paramCount = (uint32_t)fn.paramTypes.size();
    int axOccupiedUntil = -1; // instruction index until which A:X is occupied

    // Build a set of "local variable" vRegs — these have long implicit lifetimes
    // and should NOT be reused (they may be accessed via inline asm or address-of)
    std::set<uint32_t> localVarVregs;
    for (const auto& [name, vid] : fn.localNames) localVarVregs.insert(vid);
    for (uint32_t i = 0; i < paramCount; i++) localVarVregs.insert(i);

    // PHASE 1: Pre-allocate frame slots for non-register local variables
    // This prevents temporaries from using frame slots that should belong to locals
    // Process locals in declaration order first (parameters, then locals from localNames)
    std::vector<uint32_t> localVidOrder;
    for (uint32_t i = 0; i < paramCount; i++) {
        localVidOrder.push_back(i);
    }
    for (const auto& [name, vid] : fn.localNames) {
        localVidOrder.push_back(vid);
    }

    // Sort non-array locals before array locals to pack small objects first
    // This prevents temporaries from being allocated between small and large locals
    std::stable_sort(localVidOrder.begin() + paramCount, localVidOrder.end(),
        [&fn](uint32_t a, uint32_t b) {
            bool aIsArray = fn.vregSizes.count(a) > 0;
            bool bIsArray = fn.vregSizes.count(b) > 0;
            if (aIsArray != bIsArray) return !aIsArray;  // non-arrays before arrays
            if (aIsArray && bIsArray) {
                // Both arrays: sort by size, smaller first
                int aSize = fn.vregSizes.at(a);
                int bSize = fn.vregSizes.at(b);
                return aSize < bSize;
            }
            return false;  // maintain original order for non-arrays
        });

    for (uint32_t vid : localVidOrder) {
        if (registerVregs_.count(vid)) continue;        // Skip register vars (handle in phase 2)

        // Find this vreg's live range to get type info
        auto it = std::find_if(ranges_.begin(), ranges_.end(),
            [vid](const LiveRange& lr) { return lr.vregId == vid; });

        // Determine type: prefer explicit type info, fall back to live range type
        ir::Type vtype = ir::Type::I16;  // default
        if (fn.vregTypes.count(vid)) {
            vtype = fn.vregTypes.at(vid);
        } else if (it != ranges_.end()) {
            vtype = it->type;
        }

        // Allocate frame slot for this local variable
        int fsize = ir::typeSize(vtype);
        if (fsize < 2) fsize = 2;
        if (fn.vregSizes.count(vid)) {
            fsize = fn.vregSizes.at(vid);
        }

        int foff = allocFrameSlot(vtype, fsize);
        allocs_[vid] = {VRegAllocator::IN_FRAME, foff, vtype};
        frameAllocMap[vid] = {foff, fsize};
    }

    // PHASE 2: Allocate temporaries and register variables
    for (auto& lr : ranges_) {
        // Skip if already allocated in phase 1
        if (allocs_.count(lr.vregId)) continue;

        // Skip non-register locals — they were pre-allocated in phase 1
        if (localVarVregs.count(lr.vregId) && !registerVregs_.count(lr.vregId)) {
            continue;  // Skip non-register locals
        }

        int span = lr.lastUse - lr.firstDef;

        // Expire ZP and frame slots for temporaries whose live ranges have ended.
        // Only expire non-local-variable vRegs (temporaries are safe to reuse).
        {
            std::vector<uint32_t> expired;
            for (auto& [vid, zpAddr] : zpAllocMap) {
                if (localVarVregs.count(vid)) continue;
                for (auto& r : ranges_) {
                    if (r.vregId == vid && r.lastUse < lr.firstDef) {
                        expired.push_back(vid);
                        freeZpSlot(zpAddr, allocs_[vid].type);
                        break;
                    }
                }
            }
            for (auto vid : expired) zpAllocMap.erase(vid);
        }
        // Expire frame slots
        {
            std::vector<uint32_t> expired;
            for (auto& [vid, fsi] : frameAllocMap) {
                if (localVarVregs.count(vid)) continue;
                for (auto& r : ranges_) {
                    if (r.vregId == vid && r.lastUse < lr.firstDef) {
                        expired.push_back(vid);
                        freeFrameSlot(fsi.offset, fsi.size);
                        break;
                    }
                }
            }
            for (auto vid : expired) frameAllocMap.erase(vid);
        }

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
            int foff = allocFrameSlot(lr.type, size);
            allocs_[lr.vregId] = {IN_FRAME, foff, lr.type};
            frameAllocMap[lr.vregId] = {foff, size < 2 ? 2 : size};
            continue;
        }

        // Disable IN_AX allocation: src2MemOperand for FRAME vRegs uses
        // ldax.fp which clobbers AX, destroying IN_AX values between
        // their definition and use. Use ZP or FRAME only.
        bool canUseAX = false;

        // Bug #179 fix: Locals (including parameters) MUST go to FRAME, never ZP.
        // They may be accessed by linked functions that expect frame offsets,
        // and reusing ZP across different locals causes address mismatches.
        // EXCEPTION: Register variables (register keyword) are explicitly marked for ZP
        bool isLocal = localVarVregs.count(lr.vregId) > 0;
        bool isRegisterVar = registerVregs_.count(lr.vregId) > 0;

        if (canUseAX && span <= 1 && !isLocal) {
            // Short-lived, no conflict — keep in A:X (only for non-locals)
            allocs_[lr.vregId] = {IN_AX, 0, lr.type};
            axOccupiedUntil = lr.lastUse;
            for (int i = lr.firstDef; i <= lr.lastUse && i < (int)axState_.size(); i++) {
                axState_[i] = (int)lr.vregId;
            }
        } else if (!crossesCall && (!isLocal || isRegisterVar)) {
            // Try ZP for:
            // - medium-lived non-local vRegs that don't cross calls, OR
            // - register variables (even if they're locals, register keyword overrides frame allocation)
            int zpAddr = allocZpSlot(lr.type);
            if (zpAddr >= 0) {
                allocs_[lr.vregId] = {IN_ZP, zpAddr, lr.type};
                zpAllocMap[lr.vregId] = zpAddr;
            } else {
                // ZP pool exhausted, fall back to FRAME
                int foff = allocFrameSlot(lr.type);
                int fsize = ir::typeSize(lr.type); if (fsize < 2) fsize = 2;
                allocs_[lr.vregId] = {IN_FRAME, foff, lr.type};
                frameAllocMap[lr.vregId] = {foff, fsize};
            }
        } else {
            // Either: crosses a call (ZP is clobbered), or is a local (must use frame)
            // In both cases, locals and call-crossing vRegs go to FRAME
            int foff = allocFrameSlot(lr.type);
            int fsize = ir::typeSize(lr.type); if (fsize < 2) fsize = 2;
            allocs_[lr.vregId] = {IN_FRAME, foff, lr.type};
            frameAllocMap[lr.vregId] = {foff, fsize};
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
    extendLiveRangesAcrossLoops(fn);
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
