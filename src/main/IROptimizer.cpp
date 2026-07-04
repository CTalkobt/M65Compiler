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
        // CONST/FCONST excluded: cheap to recompute (lda #imm), CSE wastes ZP slots
        // ADDR_LOCAL excluded: frame-offset computation, CSE extends live range unnecessarily
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
        case Op::ADDR_GLOBAL: case Op::ADDR_LABEL: case Op::ADDR_ELEM:
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
}

// ============================================================================
// Strength Reduction
// ============================================================================

void optimizeStrengthReduction(Module& mod) {
    for (auto& fn : mod.functions) {
        // Build constant value map: vreg → immediate value
        // (for looking up CONST operands of MUL/DIV/MOD)
        std::map<uint32_t, int64_t> constVals;
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.op == Op::CONST && inst.dest.isVreg() && inst.src1.isImm()) {
                    constVals[inst.dest.vregId] = inst.src1.immVal;
                }
            }
        }

        // Rewrite instructions
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                // x / x → 1, x % x → 0 (when both operands are the same vreg)
                if (inst.src1.isVreg() && inst.src2.isVreg() &&
                    inst.src1.vregId == inst.src2.vregId) {
                    if (inst.op == Op::DIV || inst.op == Op::DIV_U) {
                        inst.op = Op::CONST;
                        inst.src1 = Operand::imm(1, inst.resultType);
                        inst.src2 = Operand::none();
                        continue;
                    }
                    if (inst.op == Op::MOD || inst.op == Op::MOD_U) {
                        inst.op = Op::CONST;
                        inst.src1 = Operand::imm(0, inst.resultType);
                        inst.src2 = Operand::none();
                        continue;
                    }
                }

                // Only handle ops with a vreg src2 that resolves to a constant
                if (!inst.src2.isVreg()) continue;
                auto it = constVals.find(inst.src2.vregId);
                if (it == constVals.end()) continue;
                int64_t val = it->second;

                switch (inst.op) {
                    case Op::MUL:
                    case Op::MUL_U: {
                        if (val == 0) {
                            // x * 0 → 0
                            inst.op = Op::CONST;
                            inst.src1 = Operand::imm(0, inst.resultType);
                            inst.src2 = Operand::none();
                        } else if (val == 1) {
                            // x * 1 → copy x
                            inst.op = Op::COPY;
                            inst.src2 = Operand::none();
                        } else if (val > 0 && (val & (val - 1)) == 0) {
                            // x * 2^n → x << n
                            int shift = 0;
                            int64_t v = val;
                            while (v > 1) { v >>= 1; shift++; }
                            inst.op = Op::SHL;
                            inst.src2 = Operand::imm(shift, inst.resultType);
                        }
                        break;
                    }
                    case Op::DIV:
                    case Op::DIV_U: {
                        if (val == 1) {
                            // x / 1 → copy x
                            inst.op = Op::COPY;
                            inst.src2 = Operand::none();
                        } else if (inst.op == Op::DIV_U && val > 0 && (val & (val - 1)) == 0) {
                            // x / 2^n → x >> n (unsigned only)
                            int shift = 0;
                            int64_t v = val;
                            while (v > 1) { v >>= 1; shift++; }
                            inst.op = Op::SHR;
                            inst.src2 = Operand::imm(shift, inst.resultType);
                        }
                        break;
                    }
                    case Op::MOD:
                    case Op::MOD_U: {
                        if (val == 1) {
                            // x % 1 → 0
                            inst.op = Op::CONST;
                            inst.src1 = Operand::imm(0, inst.resultType);
                            inst.src2 = Operand::none();
                        } else if (inst.op == Op::MOD_U && val > 0 && (val & (val - 1)) == 0) {
                            // x % 2^n → x & (2^n - 1) (unsigned only)
                            inst.op = Op::AND;
                            inst.src2 = Operand::imm(val - 1, inst.resultType);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
}

// ============================================================================
// Algebraic Simplification
// ============================================================================

void optimizeAlgebraic(Module& mod) {
    for (auto& fn : mod.functions) {
        // Build constant value map
        std::map<uint32_t, int64_t> constVals;
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.op == Op::CONST && inst.dest.isVreg() && inst.src1.isImm()) {
                    constVals[inst.dest.vregId] = inst.src1.immVal;
                }
            }
        }

        // Helper: get constant value of an operand, if known
        auto getConst = [&](const Operand& op, int64_t& out) -> bool {
            if (op.isImm()) { out = op.immVal; return true; }
            if (op.isVreg()) {
                auto it = constVals.find(op.vregId);
                if (it != constVals.end()) { out = it->second; return true; }
            }
            return false;
        };

        // Full-width mask for a given type
        auto fullMask = [](Type t) -> int64_t {
            switch (t) {
                case Type::I8:  return 0xFF;
                case Type::I16: case Type::PTR: return 0xFFFF;
                case Type::I32: return 0xFFFFFFFF;
                default: return 0xFFFF;
            }
        };

        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                // Same-operand patterns (x op x)
                if (inst.src1.isVreg() && inst.src2.isVreg() &&
                    inst.src1.vregId == inst.src2.vregId) {
                    switch (inst.op) {
                        case Op::SUB:   // x - x → 0
                        case Op::XOR:   // x ^ x → 0
                            inst.op = Op::CONST;
                            inst.src1 = Operand::imm(0, inst.resultType);
                            inst.src2 = Operand::none();
                            continue;
                        case Op::AND:   // x & x → x
                        case Op::OR:    // x | x → x
                            inst.op = Op::COPY;
                            inst.src2 = Operand::none();
                            continue;
                        default: break;
                    }
                }

                // Constant-operand patterns: check src2
                int64_t val2;
                if (getConst(inst.src2, val2)) {
                    switch (inst.op) {
                        case Op::ADD:
                        case Op::SUB:
                            if (val2 == 0) {
                                // x + 0 → x, x - 0 → x
                                inst.op = Op::COPY;
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::AND:
                            if (val2 == 0) {
                                // x & 0 → 0
                                inst.op = Op::CONST;
                                inst.src1 = Operand::imm(0, inst.resultType);
                                inst.src2 = Operand::none();
                            } else if (val2 == fullMask(inst.resultType)) {
                                // x & 0xFFFF (I16) → x
                                inst.op = Op::COPY;
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::OR:
                            if (val2 == 0) {
                                // x | 0 → x
                                inst.op = Op::COPY;
                                inst.src2 = Operand::none();
                            } else if (val2 == fullMask(inst.resultType)) {
                                // x | 0xFFFF → 0xFFFF
                                inst.op = Op::CONST;
                                inst.src1 = Operand::imm(val2, inst.resultType);
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::XOR:
                            if (val2 == 0) {
                                // x ^ 0 → x
                                inst.op = Op::COPY;
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::SHL:
                        case Op::SHR:
                        case Op::LSL:
                        case Op::LSR:
                        case Op::ASR:
                            if (val2 == 0) {
                                // x << 0 → x, x >> 0 → x
                                inst.op = Op::COPY;
                                inst.src2 = Operand::none();
                            }
                            break;
                        default: break;
                    }
                }

                // Constant-operand patterns: check src1 (for commutative ops)
                int64_t val1;
                if (getConst(inst.src1, val1)) {
                    switch (inst.op) {
                        case Op::ADD:
                            if (val1 == 0) {
                                // 0 + x → x
                                inst.op = Op::COPY;
                                inst.src1 = inst.src2;
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::AND:
                            if (val1 == 0) {
                                // 0 & x → 0
                                inst.op = Op::CONST;
                                inst.src1 = Operand::imm(0, inst.resultType);
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::OR:
                            if (val1 == 0) {
                                // 0 | x → x
                                inst.op = Op::COPY;
                                inst.src1 = inst.src2;
                                inst.src2 = Operand::none();
                            }
                            break;
                        default: break;
                    }
                }
            }
        }
    }
}

// ============================================================================
// ADDR_ELEM Fusion with LOAD/STORE
// ============================================================================

void optimizeAddrElemFusion(Module& mod) {
    // When ADDR_ELEM result is used exactly once by the immediately following
    // LOAD or STORE, embed the ADDR_ELEM info into the LOAD/STORE instruction's
    // args and remove the ADDR_ELEM. This allows the codegen to compute the
    // address inline without allocating a ZP slot for the intermediate pointer.
    //
    // Pattern:
    //   %5 = addr_elem %base, %idx, stride
    //   %6 = load i16 %5             (or: store i8 %val, %5)
    //
    // After fusion:
    //   %5 = nop                      (removed)
    //   %6 = load i16 %5             (with args[0]=base, args[1]=idx, args[2]=stride)
    //   The codegen detects args.size() >= 3 on LOAD/STORE as "fused addr_elem".

    for (auto& fn : mod.functions) {
        // Build use counts for each vreg
        std::map<uint32_t, int> useCount;
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.src1.isVreg()) useCount[inst.src1.vregId]++;
                if (inst.src2.isVreg()) useCount[inst.src2.vregId]++;
                for (auto& arg : inst.args)
                    if (arg.isVreg()) useCount[arg.vregId]++;
            }
        }

        for (auto& block : fn.blocks) {
            for (size_t i = 0; i + 1 < block.insts.size(); i++) {
                auto& addrInst = block.insts[i];
                auto& nextInst = block.insts[i + 1];

                if (addrInst.op != Op::ADDR_ELEM) continue;
                if (!addrInst.dest.isVreg()) continue;
                uint32_t addrVreg = addrInst.dest.vregId;

                // Must be used exactly once
                if (useCount[addrVreg] != 1) continue;
                // Don't fuse local slot vregs
                if (fn.localSlotVregs.count(addrVreg)) continue;

                // Next instruction must be LOAD or STORE using this vreg as address
                bool isLoad = (nextInst.op == Op::LOAD && nextInst.src1.isVreg() &&
                               nextInst.src1.vregId == addrVreg);
                bool isStore = (nextInst.op == Op::STORE && nextInst.src2.isVreg() &&
                                nextInst.src2.vregId == addrVreg);
                if (!isLoad && !isStore) continue;

                // Fuse: copy ADDR_ELEM params into the LOAD/STORE args
                int elemSize = (addrInst.args.size() > 0) ? (int)addrInst.args[0].immVal : 2;
                nextInst.args.clear();
                nextInst.args.push_back(addrInst.src1); // base
                nextInst.args.push_back(addrInst.src2); // index
                nextInst.args.push_back(Operand::imm(elemSize, Type::I16)); // stride

                // Remove the ADDR_ELEM
                addrInst.op = Op::NOP;
                addrInst.dest = Operand::none();
                addrInst.src1 = Operand::none();
                addrInst.src2 = Operand::none();
                addrInst.args.clear();
            }
        }
    }
}

// ============================================================================
// COPY Chain Elimination
// ============================================================================

void optimizeCopyChains(Module& mod) {
    for (auto& fn : mod.functions) {
        // Build forwarding map: follow COPY chains to the root operand.
        // %a = COPY %b; %c = COPY %a → %c should reference %b directly.
        std::map<uint32_t, Operand> copyRoot;
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.op == Op::COPY && inst.dest.isVreg() && inst.src1.isVreg()) {
                    // Follow chain to root
                    Operand root = inst.src1;
                    int depth = 16;
                    while (root.isVreg() && --depth > 0) {
                        auto it = copyRoot.find(root.vregId);
                        if (it != copyRoot.end())
                            root = it->second;
                        else
                            break;
                    }
                    copyRoot[inst.dest.vregId] = root;
                }
            }
        }

        if (copyRoot.empty()) return;

        // Rewrite all operands using the forwarding map
        auto rewrite = [&](Operand& op) {
            if (op.isVreg()) {
                auto it = copyRoot.find(op.vregId);
                if (it != copyRoot.end()) {
                    op.vregId = it->second.vregId;
                }
            }
        };

        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                rewrite(inst.src1);
                rewrite(inst.src2);
                for (auto& arg : inst.args) rewrite(arg);
                for (auto& phi : inst.phiIncoming) rewrite(phi.first);
            }
        }

        // Mark COPYs whose dest is no longer referenced as NOP
        std::set<uint32_t> usedVregs;
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.src1.isVreg()) usedVregs.insert(inst.src1.vregId);
                if (inst.src2.isVreg()) usedVregs.insert(inst.src2.vregId);
                for (auto& arg : inst.args)
                    if (arg.isVreg()) usedVregs.insert(arg.vregId);
                for (auto& phi : inst.phiIncoming)
                    if (phi.first.isVreg()) usedVregs.insert(phi.first.vregId);
            }
        }
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.op == Op::COPY && inst.dest.isVreg() &&
                    !usedVregs.count(inst.dest.vregId) &&
                    !fn.localSlotVregs.count(inst.dest.vregId)) {
                    inst.op = Op::NOP;
                    inst.src1 = Operand::none();
                    inst.src2 = Operand::none();
                    inst.dest = Operand::none();
                }
            }
        }
    }
}

// ============================================================================
// Type Narrowing (I16/I32 → I8)
// ============================================================================

void optimizeTypeNarrowing(Module& mod) {
    for (auto& fn : mod.functions) {
        // Build use map: for each vreg, collect all instructions that use it
        // and what role (src1, src2, arg, store-value)
        struct UseInfo {
            Inst* inst;
            enum Role { SRC1, SRC2, ARG, STORE_VAL, PHI, OTHER };
            Role role;
        };
        std::map<uint32_t, std::vector<UseInfo>> uses;

        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.src1.isVreg())
                    uses[inst.src1.vregId].push_back({&inst, UseInfo::SRC1});
                if (inst.src2.isVreg())
                    uses[inst.src2.vregId].push_back({&inst, UseInfo::SRC2});
                for (auto& arg : inst.args) {
                    if (arg.isVreg())
                        uses[arg.vregId].push_back({&inst, UseInfo::ARG});
                }
            }
        }

        // Narrowable arithmetic ops (safe to compute in I8 when result is truncated)
        auto isNarrowable = [](Op op) -> bool {
            // SUB and NEG excluded: result sign matters (char subtraction
            // must produce signed int result per C integer promotion rules)
            switch (op) {
                case Op::ADD:
                case Op::MUL: case Op::MUL_U:
                case Op::AND: case Op::OR: case Op::XOR:
                case Op::SHL: case Op::SHR:
                case Op::NOT:
                    return true;
                default:
                    return false;
            }
        };

        // Check if ALL uses of a vreg are TRUNC to I8
        auto allUsesTruncI8 = [&](uint32_t vregId) -> bool {
            auto it = uses.find(vregId);
            if (it == uses.end()) return false;
            for (auto& u : it->second) {
                if (u.inst->op == Op::TRUNC && u.inst->resultType == Type::I8 &&
                    u.role == UseInfo::SRC1) {
                    continue; // this use is a TRUNC to I8 — OK
                }
                return false; // non-TRUNC use — can't narrow
            }
            return true;
        };

        // Pass: narrow I16/I32 ops whose result is only used via TRUNC I8
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (!inst.dest.isVreg()) continue;
                if (inst.resultType != Type::I16 && inst.resultType != Type::I32) continue;
                if (!isNarrowable(inst.op)) continue;

                // Don't narrow local slot vregs
                if (fn.localSlotVregs.count(inst.dest.vregId)) continue;

                if (!allUsesTruncI8(inst.dest.vregId)) continue;

                // Narrow: change result type to I8
                inst.resultType = Type::I8;

                // Convert each TRUNC consumer to COPY (the result is already I8)
                auto& useList = uses[inst.dest.vregId];
                for (auto& u : useList) {
                    if (u.inst->op == Op::TRUNC && u.inst->resultType == Type::I8) {
                        u.inst->op = Op::COPY;
                        u.inst->resultType = Type::I8;
                    }
                }
            }
        }

        // Pass 2: Eliminate redundant ZEXT/SEXT where the extended value is
        // only used in I8 context (TRUNC back to I8)
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.op != Op::ZEXT && inst.op != Op::SEXT) continue;
                if (!inst.dest.isVreg()) continue;
                // Check if src1 is I8 and all uses are TRUNC back to I8
                if (inst.src1.type != Type::I8) continue;

                if (!allUsesTruncI8(inst.dest.vregId)) continue;

                // The ZEXT/SEXT is pointless — replace with COPY
                inst.op = Op::COPY;
                inst.resultType = Type::I8;

                // Convert TRUNC consumers to COPY
                auto& useList = uses[inst.dest.vregId];
                for (auto& u : useList) {
                    if (u.inst->op == Op::TRUNC && u.inst->resultType == Type::I8) {
                        u.inst->op = Op::COPY;
                        u.inst->resultType = Type::I8;
                    }
                }
            }
        }
    }
}

// ============================================================================
// Branch Folding and Dead Block Elimination
// ============================================================================

void optimizeBranchFold(Module& mod) {
    for (auto& fn : mod.functions) {
        bool changed = true;
        int maxIter = 16;
        while (changed && --maxIter > 0) {
            changed = false;

            // Build constant value map
            std::map<uint32_t, int64_t> constVals;
            for (auto& block : fn.blocks) {
                for (auto& inst : block.insts) {
                    if (inst.op == Op::CONST && inst.dest.isVreg() && inst.src1.isImm())
                        constVals[inst.dest.vregId] = inst.src1.immVal;
                }
            }

            // Pass 1: Fold BR_COND on constant condition
            // If target is the next block, remove the branch entirely (fallthrough).
            // Otherwise, replace with unconditional BR.
            for (size_t bi = 0; bi < fn.blocks.size(); bi++) {
                for (auto it = fn.blocks[bi].insts.begin(); it != fn.blocks[bi].insts.end(); ++it) {
                    if (it->op != Op::BR_COND) continue;
                    if (!it->src1.isVreg()) continue;
                    auto cv = constVals.find(it->src1.vregId);
                    if (cv == constVals.end()) continue;

                    std::string target = (cv->second != 0) ? it->dest.name : it->src2.name;

                    // If target is the next block, just remove the branch (fallthrough)
                    if (bi + 1 < fn.blocks.size() && fn.blocks[bi + 1].label == target) {
                        fn.blocks[bi].insts.erase(it);
                    } else {
                        it->op = Op::BR;
                        it->src1 = Operand::label(target);
                        it->src2 = Operand::none();
                        it->dest = Operand::none();
                    }
                    changed = true;
                    break; // block's terminator changed, move on
                }
            }

            // Pass 2: Thread jump chains — BR to a block that only contains BR
            {
                // Build label → block index map
                std::map<std::string, size_t> blockIndex;
                for (size_t i = 0; i < fn.blocks.size(); i++)
                    blockIndex[fn.blocks[i].label] = i;

                for (auto& block : fn.blocks) {
                    for (auto& inst : block.insts) {
                        auto threadLabel = [&](std::string& label) {
                            int depth = 8; // limit chain depth
                            while (--depth > 0) {
                                auto bi = blockIndex.find(label);
                                if (bi == blockIndex.end()) break;
                                auto& target = fn.blocks[bi->second];
                                // Block contains only a single BR instruction?
                                if (target.insts.size() == 1 && target.insts[0].op == Op::BR &&
                                    !target.insts[0].src1.name.empty() &&
                                    target.insts[0].src1.name != label) {
                                    label = target.insts[0].src1.name;
                                    changed = true;
                                } else {
                                    break;
                                }
                            }
                        };

                        if (inst.op == Op::BR && !inst.src1.name.empty()) {
                            threadLabel(inst.src1.name);
                        } else if (inst.op == Op::BR_COND) {
                            if (!inst.dest.name.empty()) threadLabel(inst.dest.name);
                            if (!inst.src2.name.empty()) threadLabel(inst.src2.name);
                        }
                    }
                }
            }

            // Pass 2b: Remove redundant BR to next block (fallthrough)
            for (size_t bi = 0; bi + 1 < fn.blocks.size(); bi++) {
                auto& insts = fn.blocks[bi].insts;
                if (!insts.empty() && insts.back().op == Op::BR &&
                    insts.back().src1.name == fn.blocks[bi + 1].label) {
                    insts.pop_back();
                    changed = true;
                }
            }

            // Pass 3: Remove unreachable blocks (no predecessors except entry)
            if (fn.blocks.size() > 1) {
                // Collect all referenced labels
                std::set<std::string> referenced;
                referenced.insert(fn.blocks[0].label); // entry is always reachable

                for (size_t i = 0; i < fn.blocks.size(); i++) {
                    auto& block = fn.blocks[i];
                    bool hasTerminator = false;
                    for (auto& inst : block.insts) {
                        if (inst.op == Op::BR) {
                            referenced.insert(inst.src1.name);
                            hasTerminator = true;
                        } else if (inst.op == Op::BR_COND) {
                            referenced.insert(inst.dest.name);
                            referenced.insert(inst.src2.name);
                            hasTerminator = true;
                        } else if (inst.op == Op::SWITCH) {
                            referenced.insert(inst.src2.name);
                            for (auto& sc : inst.switchCases)
                                referenced.insert(sc.second);
                            hasTerminator = true;
                        } else if (inst.op == Op::RET || inst.op == Op::RET_VOID) {
                            hasTerminator = true;
                        }
                    }
                    // Fallthrough: if no terminator, the next block is reachable
                    if (!hasTerminator && i + 1 < fn.blocks.size()) {
                        referenced.insert(fn.blocks[i + 1].label);
                    }
                }

                // Remove blocks that aren't referenced
                auto it = fn.blocks.begin();
                while (it != fn.blocks.end()) {
                    if (!referenced.count(it->label)) {
                        it = fn.blocks.erase(it);
                        changed = true;
                    } else {
                        ++it;
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

            // Note: hoisted vregs' liveness across the loop is handled by
            // VRegAllocator::extendLiveRangesAcrossLoops(), which extends
            // live ranges that overlap loop bodies to cover the entire loop.

                restart = true;
                break; // Indices shifted — restart CFG analysis
            }
        } // while (restart)
    }
}

} // namespace ir
