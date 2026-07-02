#include "IROptimizer.hpp"
#include <algorithm>
#include <map>
#include <vector>

namespace ir {

namespace {

struct ExprKey {
    Op op;
    Operand src1;
    Operand src2;
    Type type;
    uint8_t byteWidth;
    std::string name;

    bool operator==(const ExprKey& o) const {
        return op == o.op &&
               src1.kind == o.src1.kind && src1.vregId == o.src1.vregId && src1.immVal == o.src1.immVal && src1.name == o.src1.name &&
               src2.kind == o.src2.kind && src2.vregId == o.src2.vregId && src2.immVal == o.src2.immVal && src2.name == o.src2.name &&
               type == o.type &&
               byteWidth == o.byteWidth &&
               name == o.name;
    }
};

bool isCSECandidate(Op op) {
    switch (op) {
        case Op::CONST:
        case Op::ADD: case Op::SUB: case Op::MUL: case Op::DIV: case Op::MOD:
        case Op::MUL_U: case Op::DIV_U: case Op::MOD_U:
        case Op::AND: case Op::OR: case Op::XOR:
        case Op::LSL: case Op::LSR: case Op::ASR:
        case Op::SHL: case Op::SHR:
        case Op::NEG: case Op::NOT:
        case Op::CMP_EQ: case Op::CMP_NE:
        case Op::CMP_LT: case Op::CMP_LE: case Op::CMP_GT: case Op::CMP_GE:
        case Op::CMP_LTU: case Op::CMP_LEU: case Op::CMP_GTU: case Op::CMP_GEU:
        case Op::SEXT: case Op::ZEXT: case Op::TRUNC:
        case Op::ADDR_GLOBAL: case Op::ADDR_LOCAL: case Op::ADDR_LABEL: case Op::ADDR_ELEM:
        case Op::LOAD: case Op::LOAD_ZP:
            return true;
        default:
            return false;
    }
}

void propagateOperand(Operand& op, const std::map<uint32_t, Operand>& copies) {
    if (op.isVreg()) {
        auto it = copies.find(op.vregId);
        if (it != copies.end()) {
            op = it->second;
            // Recursively resolve
            propagateOperand(op, copies);
        }
    }
}

} // namespace

void optimizeCSE(Module& mod) {
    for (auto& fn : mod.functions) {
        // std::printf("--- Optimizing function: '%s' ---\n", fn.name.c_str());
        for (auto& block : fn.blocks) {
            std::map<uint32_t, Operand> copies;
            std::vector<std::pair<ExprKey, Operand>> activeExprs;

            for (auto& inst : block.insts) {
                // std::printf("  Before: op=%d, dest=%u, src1=%u, src2=%u\n",
                //             (int)inst.op, inst.dest.vregId, inst.src1.vregId, inst.src2.vregId);

                // 1. Apply copy propagation to operands
                propagateOperand(inst.src1, copies);
                propagateOperand(inst.src2, copies);
                for (auto& arg : inst.args) {
                    propagateOperand(arg, copies);
                }
                for (auto& pair : inst.phiIncoming) {
                    propagateOperand(pair.first, copies);
                }

                // std::printf("  After Prop: op=%d, dest=%u, src1=%u, src2=%u\n",
                //             (int)inst.op, inst.dest.vregId, inst.src1.vregId, inst.src2.vregId);

                // 2. Handle memory stores invalidating load expressions
                if (inst.op == Op::STORE || inst.op == Op::STORE_ZP ||
                    inst.op == Op::CALL || inst.op == Op::CALL_INDIRECT || inst.op == Op::CALL_VOID ||
                    inst.op == Op::ASM_INLINE || inst.op == Op::CPU_REG_WRITE || inst.op == Op::CPU_FLAG_WRITE) {
                    
                    activeExprs.erase(
                        std::remove_if(activeExprs.begin(), activeExprs.end(),
                                       [](const auto& pair) {
                                           return pair.first.op == Op::LOAD || pair.first.op == Op::LOAD_ZP;
                                       }),
                        activeExprs.end()
                    );
                }

                // 3. Record new copies directly
                if (inst.op == Op::COPY) {
                    if (inst.dest.isVreg()) {
                        copies[inst.dest.vregId] = inst.src1;
                        // std::printf("    Recorded copy: %u -> %u\n", inst.dest.vregId, inst.src1.vregId);
                    }
                    continue;
                }

                // 4. Try to match expression for CSE
                if (isCSECandidate(inst.op) && inst.dest.isVreg()) {
                    ExprKey key;
                    key.op = inst.op;
                    key.src1 = inst.src1;
                    key.src2 = inst.src2;
                    key.type = inst.resultType;
                    key.byteWidth = inst.resultByteWidth;
                    
                    // If call or memory-indexed, carry label or symbol info
                    if (inst.op == Op::ADDR_GLOBAL || inst.op == Op::ADDR_LABEL) {
                        key.name = inst.src1.name;
                    }

                    // Check if already computed in this block
                    bool found = false;
                    for (const auto& pair : activeExprs) {
                        if (pair.first == key) {
                            // Match found! Replace instruction with COPY
                            inst.op = Op::COPY;
                            inst.src1 = pair.second;
                            inst.src2 = Operand::none();
                            copies[inst.dest.vregId] = pair.second;
                            // std::printf("    Matched CSE! Replace with copy of %d\n", pair.second.vregId);
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        activeExprs.push_back({key, inst.dest});
                    }
                }
            }
        }
    }
    // Now run it on all functions actually
    for (auto& fn : mod.functions) {
        if (fn.name == "_main" || fn.name == "main") continue;
        for (auto& block : fn.blocks) {
            std::map<uint32_t, Operand> copies;
            std::vector<std::pair<ExprKey, Operand>> activeExprs;

            for (auto& inst : block.insts) {
                propagateOperand(inst.src1, copies);
                propagateOperand(inst.src2, copies);
                for (auto& arg : inst.args) {
                    propagateOperand(arg, copies);
                }
                for (auto& pair : inst.phiIncoming) {
                    propagateOperand(pair.first, copies);
                }
                if (inst.op == Op::STORE || inst.op == Op::STORE_ZP ||
                    inst.op == Op::CALL || inst.op == Op::CALL_INDIRECT || inst.op == Op::CALL_VOID ||
                    inst.op == Op::ASM_INLINE || inst.op == Op::CPU_REG_WRITE || inst.op == Op::CPU_FLAG_WRITE) {
                    activeExprs.erase(
                        std::remove_if(activeExprs.begin(), activeExprs.end(),
                                       [](const auto& pair) {
                                           return pair.first.op == Op::LOAD || pair.first.op == Op::LOAD_ZP;
                                       }),
                        activeExprs.end()
                    );
                }
                if (inst.op == Op::COPY) {
                    if (inst.dest.isVreg()) {
                        copies[inst.dest.vregId] = inst.src1;
                    }
                    continue;
                }
                if (isCSECandidate(inst.op) && inst.dest.isVreg()) {
                    ExprKey key;
                    key.op = inst.op;
                    key.src1 = inst.src1;
                    key.src2 = inst.src2;
                    key.type = inst.resultType;
                    key.byteWidth = inst.resultByteWidth;
                    if (inst.op == Op::ADDR_GLOBAL || inst.op == Op::ADDR_LABEL) {
                        key.name = inst.src1.name;
                    }
                    bool found = false;
                    for (const auto& pair : activeExprs) {
                        if (pair.first == key) {
                            inst.op = Op::COPY;
                            inst.src1 = pair.second;
                            inst.src2 = Operand::none();
                            copies[inst.dest.vregId] = pair.second;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        activeExprs.push_back({key, inst.dest});
                    }
                }
            }
        }
    }
}

// ============================================================================
// LICM — Loop-Invariant Code Motion
// ============================================================================

namespace {

// Collect branch targets from a block's terminator instruction
void collectSuccessors(const Block& block, std::vector<std::string>& succs) {
    for (const auto& inst : block.insts) {
        switch (inst.op) {
            case Op::BR:
                if (!inst.src1.name.empty())
                    succs.push_back(inst.src1.name);
                break;
            case Op::BR_COND:
                // BR_COND: src1=condition, dest=true label, src2=false label
                if (!inst.dest.name.empty())
                    succs.push_back(inst.dest.name);   // true label
                if (!inst.src2.name.empty())
                    succs.push_back(inst.src2.name);   // false label
                break;
            case Op::SWITCH:
                if (!inst.src2.name.empty())
                    succs.push_back(inst.src2.name);  // default label
                for (const auto& sc : inst.switchCases)
                    succs.push_back(sc.second);
                break;
            case Op::BR_INDIRECT:
                // Computed goto — target unknown at compile time; no successors to track
                break;
            default:
                break;
        }
    }
}

// Check if an instruction is pure (no side effects) and safe to hoist
bool isHoistCandidate(Op op) {
    switch (op) {
        // CONST and FCONST are marked as hoist candidates for invariance
        // analysis (so dependents can be hoisted), but are DUPLICATED not MOVED
        // during the actual hoisting phase to avoid wasting ZP registers.
        case Op::CONST:
        case Op::FCONST:
        case Op::ADD: case Op::SUB: case Op::MUL: case Op::DIV: case Op::MOD:
        case Op::MUL_U: case Op::DIV_U: case Op::MOD_U:
        case Op::AND: case Op::OR: case Op::XOR:
        case Op::LSL: case Op::LSR: case Op::ASR:
        case Op::SHL: case Op::SHR:
        case Op::NEG: case Op::NOT:
        case Op::CMP_EQ: case Op::CMP_NE:
        case Op::CMP_LT: case Op::CMP_LE: case Op::CMP_GT: case Op::CMP_GE:
        case Op::CMP_LTU: case Op::CMP_LEU: case Op::CMP_GTU: case Op::CMP_GEU:
        case Op::SEXT: case Op::ZEXT: case Op::TRUNC:
        case Op::ADDR_GLOBAL: case Op::ADDR_LOCAL: case Op::ADDR_LABEL:
        case Op::ADDR_ELEM:
        case Op::COPY:
        case Op::FNEG:
            return true;
        default:
            return false;
    }
}

} // namespace

void optimizeLICM(Module& mod) {
    for (auto& fn : mod.functions) {
        // Restart loop: after inserting a pre-header block, indices shift,
        // so we rebuild the CFG and scan for the next back-edge.
        bool restart = true;
        int maxPasses = 32; // safety limit for deeply nested loops
        while (restart && --maxPasses > 0) {
            restart = false;
            if (fn.blocks.size() < 2) break;

        // Build block index: label → position in fn.blocks
        std::map<std::string, size_t> blockIndex;
        for (size_t i = 0; i < fn.blocks.size(); i++) {
            blockIndex[fn.blocks[i].label] = i;
        }

        // Build successor/predecessor maps
        std::vector<std::vector<size_t>> succs(fn.blocks.size());
        std::vector<std::vector<size_t>> preds(fn.blocks.size());
        for (size_t i = 0; i < fn.blocks.size(); i++) {
            std::vector<std::string> targets;
            collectSuccessors(fn.blocks[i], targets);

            // If the block has no explicit branch/return, it falls through
            // to the next block (implicit edge)
            if (targets.empty() && i + 1 < fn.blocks.size()) {
                bool hasTerminator = false;
                for (const auto& inst : fn.blocks[i].insts) {
                    if (inst.op == Op::BR || inst.op == Op::BR_COND ||
                        inst.op == Op::BR_INDIRECT || inst.op == Op::SWITCH ||
                        inst.op == Op::RET || inst.op == Op::RET_VOID) {
                        hasTerminator = true;
                        break;
                    }
                }
                if (!hasTerminator) {
                    succs[i].push_back(i + 1);
                    preds[i + 1].push_back(i);
                }
            }

            for (const auto& t : targets) {
                auto it = blockIndex.find(t);
                if (it != blockIndex.end()) {
                    succs[i].push_back(it->second);
                    preds[it->second].push_back(i);
                }
            }
        }

        // Find back-edges: edge i→j where j <= i (target is at or before source
        // in block order; works for structured code from cc45)
        struct BackEdge { size_t from; size_t to; };
        std::vector<BackEdge> backEdges;
        for (size_t i = 0; i < fn.blocks.size(); i++) {
            for (size_t s : succs[i]) {
                if (s <= i) {
                    backEdges.push_back({i, s});
                }
            }
        }

        if (backEdges.empty()) break;

        // Process back-edges. If we hoist anything, restart.
        for (const auto& be : backEdges) {
            size_t header = be.to;
            size_t tail = be.from;

            // Skip pre-header blocks we already inserted
            if (fn.blocks[header].label.find("_ph") != std::string::npos) continue;

            std::set<size_t> loopBlocks;
            loopBlocks.insert(header);

            if (header != tail) {
                // Reverse DFS from tail, collecting blocks that reach tail
                // without passing through header
                std::vector<size_t> worklist;
                loopBlocks.insert(tail);
                worklist.push_back(tail);

                while (!worklist.empty()) {
                    size_t b = worklist.back();
                    worklist.pop_back();
                    for (size_t p : preds[b]) {
                        if (loopBlocks.find(p) == loopBlocks.end()) {
                            loopBlocks.insert(p);
                            worklist.push_back(p);
                        }
                    }
                }
            }

            if (loopBlocks.size() < 2) continue; // trivial loop, skip

            // Collect all vregs defined inside the loop, and
            // local slot vregs that are STORED TO inside the loop.
            // Local slots are memory locations (stack variables). Even though the
            // slot vreg (address) is defined outside the loop, reads from mutated
            // slots are NOT loop-invariant.
            std::set<uint32_t> loopDefinedVregs;
            std::set<uint32_t> loopMutatedSlots;
            for (size_t bi : loopBlocks) {
                for (const auto& inst : fn.blocks[bi].insts) {
                    if (inst.dest.isVreg()) {
                        loopDefinedVregs.insert(inst.dest.vregId);
                    }
                    // STORE to a local slot vreg mutates that variable
                    if ((inst.op == Op::STORE || inst.op == Op::STORE_ZP) &&
                        inst.src2.isVreg() && fn.localSlotVregs.count(inst.src2.vregId)) {
                        loopMutatedSlots.insert(inst.src2.vregId);
                    }
                }
            }

            // Iteratively find loop-invariant instructions.
            // An instruction is invariant if:
            //   1. It is a pure (side-effect-free) operation
            //   2. All its source operands are either:
            //      - Constants/immediates/globals
            //      - Defined outside the loop (and not a mutated local slot)
            //      - Defined by another already-identified invariant instruction
            //   3. No source reads a local slot mutated inside the loop
            //   4. Its dest vreg is not a local slot or memory-pinned vreg
            std::set<uint32_t> invariantVregs;
            bool changed = true;
            while (changed) {
                changed = false;
                for (size_t bi : loopBlocks) {
                    for (const auto& inst : fn.blocks[bi].insts) {
                        if (!inst.dest.isVreg()) continue;
                        if (invariantVregs.count(inst.dest.vregId)) continue;
                        if (!isHoistCandidate(inst.op)) continue;

                        // Don't hoist local slot vregs (they represent stack addresses)
                        if (fn.localSlotVregs.count(inst.dest.vregId)) continue;
                        // Don't hoist memory-pinned vregs (volatile, address-taken)
                        if (fn.memoryVregs.count(inst.dest.vregId)) continue;

                        auto isInvariantOrExternal = [&](const Operand& op) -> bool {
                            if (op.isNone() || op.isImm()) return true;
                            if (op.kind == OperandKind::GLOBAL || op.kind == OperandKind::LABEL) return true;
                            if (op.isVreg()) {
                                // Local slot mutated inside the loop → value changes each iteration
                                if (loopMutatedSlots.count(op.vregId))
                                    return false;
                                // Defined outside loop?
                                if (loopDefinedVregs.find(op.vregId) == loopDefinedVregs.end())
                                    return true;
                                // Defined by another invariant instruction?
                                if (invariantVregs.count(op.vregId))
                                    return true;
                                return false;
                            }
                            return true;
                        };

                        if (isInvariantOrExternal(inst.src1) && isInvariantOrExternal(inst.src2)) {
                            // Also check args (for ADDR_ELEM etc.)
                            bool allArgsOk = true;
                            for (const auto& arg : inst.args) {
                                if (!isInvariantOrExternal(arg)) { allArgsOk = false; break; }
                            }
                            if (allArgsOk) {
                                invariantVregs.insert(inst.dest.vregId);
                                changed = true;
                            }
                        }
                    }
                }
            }

            if (invariantVregs.empty()) continue;

            // Collect invariant CONST definitions (for duplication, not movement).
            // CONST instructions stay in the loop body but are duplicated in the
            // pre-header so that hoisted dependents can reference them.
            std::map<uint32_t, Inst> constDefs;
            for (size_t bi : loopBlocks) {
                for (const auto& inst : fn.blocks[bi].insts) {
                    if ((inst.op == Op::CONST || inst.op == Op::FCONST) &&
                        inst.dest.isVreg() && invariantVregs.count(inst.dest.vregId)) {
                        constDefs[inst.dest.vregId] = inst;
                    }
                }
            }

            // Remove CONST-only invariants from the hoist set — they stay in the loop
            std::set<uint32_t> nonConstInvariants;
            for (uint32_t v : invariantVregs) {
                if (!constDefs.count(v)) nonConstInvariants.insert(v);
            }

            if (nonConstInvariants.empty()) continue;

            // Collect non-CONST invariant instructions (MOVE from loop body)
            std::vector<Inst> hoisted;
            for (size_t bi : loopBlocks) {
                auto& insts = fn.blocks[bi].insts;
                auto it = insts.begin();
                while (it != insts.end()) {
                    if (it->dest.isVreg() && nonConstInvariants.count(it->dest.vregId)) {
                        hoisted.push_back(std::move(*it));
                        it = insts.erase(it);
                    } else {
                        ++it;
                    }
                }
            }

            // For hoisted instructions that reference CONST vregs still in the
            // loop body, create fresh CONSTs in the pre-header with new vregs
            // to avoid double-definition of the same vreg.
            std::set<uint32_t> neededConsts;
            for (const auto& inst : hoisted) {
                auto checkOp = [&](const Operand& op) {
                    if (op.isVreg() && constDefs.count(op.vregId))
                        neededConsts.insert(op.vregId);
                };
                checkOp(inst.src1);
                checkOp(inst.src2);
                for (const auto& arg : inst.args) checkOp(arg);
            }

            // Allocate new vregs for duplicated CONSTs and build a rewrite map
            std::map<uint32_t, uint32_t> constRemap; // old vreg → new vreg
            std::vector<Inst> withConsts;
            for (uint32_t cv : neededConsts) {
                uint32_t newVreg = fn.allocVreg();
                constRemap[cv] = newVreg;
                Inst dup = constDefs[cv];
                dup.dest = Operand::vreg(newVreg, dup.dest.type);
                withConsts.push_back(dup);
            }

            // Rewrite operands in hoisted instructions to use new CONST vregs
            auto remapOp = [&](Operand& op) {
                if (op.isVreg()) {
                    auto it = constRemap.find(op.vregId);
                    if (it != constRemap.end())
                        op.vregId = it->second;
                }
            };
            for (auto& inst : hoisted) {
                remapOp(inst.src1);
                remapOp(inst.src2);
                for (auto& arg : inst.args) remapOp(arg);
            }

            withConsts.insert(withConsts.end(), hoisted.begin(), hoisted.end());
            hoisted = std::move(withConsts);

            // Check if a pre-header already exists (from a prior pass)
            std::string preheaderLabel = fn.blocks[header].label + "_ph";
            auto phIt = blockIndex.find(preheaderLabel);
            if (phIt != blockIndex.end()) {
                // Pre-header exists — insert hoisted instructions before its terminal BR
                auto& phBlock = fn.blocks[phIt->second];
                auto brIt = phBlock.insts.end();
                for (auto rit = phBlock.insts.begin(); rit != phBlock.insts.end(); ++rit) {
                    if (rit->op == Op::BR || rit->op == Op::BR_COND) {
                        brIt = rit;
                        break;
                    }
                }
                phBlock.insts.insert(brIt, hoisted.begin(), hoisted.end());
            } else {
                // Create a new pre-header block
                Block preheader;
                preheader.label = preheaderLabel;
                preheader.insts = std::move(hoisted);

                // Add branch from pre-header to header
                Inst brToHeader;
                brToHeader.op = Op::BR;
                brToHeader.src1 = Operand::label(fn.blocks[header].label);
                preheader.insts.push_back(brToHeader);

                // Redirect non-loop predecessors of header to the pre-header
                for (size_t pi : preds[header]) {
                    if (loopBlocks.count(pi)) continue;

                    auto& predBlock = fn.blocks[pi];
                    for (auto& inst : predBlock.insts) {
                        auto rewriteLabel = [&](Operand& op) {
                            if (op.kind == OperandKind::LABEL && op.name == fn.blocks[header].label)
                                op.name = preheaderLabel;
                        };
                        auto rewriteName = [&](std::string& name) {
                            if (name == fn.blocks[header].label)
                                name = preheaderLabel;
                        };

                        switch (inst.op) {
                            case Op::BR:
                                rewriteLabel(inst.src1);
                                break;
                            case Op::BR_COND:
                                rewriteName(inst.dest.name);
                                rewriteLabel(inst.src2);
                                break;
                            case Op::SWITCH:
                                rewriteLabel(inst.src2);
                                for (auto& sc : inst.switchCases)
                                    rewriteName(sc.second);
                                break;
                            default:
                                break;
                        }
                    }
                }

                // Insert pre-header just before the header
                fn.blocks.insert(fn.blocks.begin() + header, std::move(preheader));
            }

            // Insert NOP keep-alive references in the back-edge block for
            // hoisted vregs. This extends their linear-scan live range past
            // the entire loop body, preventing the ZP allocator from reusing
            // their slots for loop-body temporaries.
            auto& tailBlock = fn.blocks[tail < header ? tail : tail + 1]; // +1 if preheader was inserted
            // Find where the back-edge BR is and insert NOPs before it
            auto brPos = tailBlock.insts.end();
            for (auto rit = tailBlock.insts.begin(); rit != tailBlock.insts.end(); ++rit) {
                if (rit->op == Op::BR || rit->op == Op::BR_COND) {
                    brPos = rit;
                    break;
                }
            }
            for (uint32_t v : nonConstInvariants) {
                Inst nop;
                nop.op = Op::NOP;
                nop.src1 = Operand::vreg(v, ir::Type::I16);
                brPos = tailBlock.insts.insert(brPos, nop);
                ++brPos; // advance past inserted NOP
            }

                restart = true;
                break; // Indices shifted — restart CFG analysis
            }
        } // while (restart)
    }
}

} // namespace ir
