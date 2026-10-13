#include "IROptimizer.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <vector>

namespace ir {

std::ostream* optTrace = nullptr;
std::function<bool(const Function&, const char*)> passEnabled;

static bool runOn(const Function& fn, const char* pass) {
    return !passEnabled || passEnabled(fn, pass);
}

static void trace(const char* pass, const std::string& func, const std::string& action,
                  const SourceLoc& loc = {}) {
    if (optTrace) {
        *optTrace << "[" << pass << "] " << func;
        if (loc.valid()) {
            std::string f = loc.file;
            auto slash = f.rfind('/');
            if (slash != std::string::npos) f = f.substr(slash + 1);
            *optTrace << " (" << f << ":" << loc.line << ")";
        }
        *optTrace << ": " << action << "\n";
    }
}

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
            // The use keeps its own type: a copy may change the width
            // (copy i8 of an i16 value is its low byte)
            Type useType = op.type;
            op = it->second;
            if (op.isVreg()) op.type = useType;
            // Recursively resolve
            propagateOperand(op, copies);
        }
    }
}


// Vregs that are constants: defined once, by a CONST, never stored to and not
// a variable's slot (vregs are not SSA: a && b's result is CONST 0 on one path)
std::map<uint32_t, int64_t> singleDefConsts(const Function& fn) {
    std::map<uint32_t, int64_t> consts;
    std::map<uint32_t, int> defs;
    for (const auto& b : fn.blocks)
        for (const auto& i : b.insts) {
            if (i.dest.isVreg()) defs[i.dest.vregId]++;
            if ((i.op == Op::STORE || i.op == Op::STORE_ZP) && i.src2.isVreg()) defs[i.src2.vregId] += 2;
            if (i.op == Op::CONST && i.dest.isVreg() && i.src1.isImm()) consts[i.dest.vregId] = i.src1.immVal;
        }
    for (auto it = consts.begin(); it != consts.end();) {
        if (defs[it->first] != 1 || fn.localSlotVregs.count(it->first)) it = consts.erase(it);
        else ++it;
    }
    return consts;
}
} // namespace

void optimizeCSE(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "CommonSubexprElim")) continue;
        const auto& slots = fn.localSlotVregs;
        for (auto& block : fn.blocks) {
            std::map<uint32_t, Operand> copies;
            std::vector<std::pair<ExprKey, Operand>> activeExprs;

            // Vregs are not SSA (a variable's slot vreg is assigned by STORE, a
            // temp can be defined on several paths): forget every recorded
            // expression and copy that reads or names vreg v once v changes
            auto uses = [](const Operand& o, uint32_t v) { return o.isVreg() && o.vregId == v; };
            auto invalidate = [&](uint32_t v) {
                activeExprs.erase(std::remove_if(activeExprs.begin(), activeExprs.end(),
                    [&](const auto& e) { return uses(e.first.src1, v) || uses(e.first.src2, v) || uses(e.second, v); }),
                    activeExprs.end());
                for (auto it = copies.begin(); it != copies.end();) {
                    if (it->first == v || uses(it->second, v)) it = copies.erase(it);
                    else ++it;
                }
            };
            // A store through a pointer, a call or inline asm may change any
            // variable (its address may have been taken)
            auto invalidateVariables = [&]() {
                for (uint32_t v : slots) invalidate(v);
            };

            for (auto& inst : block.insts) {
                // 1. Apply copy propagation to operands. A variable's slot vreg
                // as the address of a LOAD/STORE (or ADDR_LOCAL's operand) names
                // the variable itself, so a copy of it is not propagated there
                bool src1IsAddr = inst.op == Op::LOAD || inst.op == Op::LOAD_ZP || inst.op == Op::ADDR_LOCAL;
                bool src2IsAddr = inst.op == Op::STORE || inst.op == Op::STORE_ZP;
                auto prop = [&](Operand& op, bool addrRole) {
                    Operand before = op;
                    propagateOperand(op, copies);
                    if (addrRole && op.isVreg() && slots.count(op.vregId)) op = before;
                };
                prop(inst.src1, src1IsAddr);
                prop(inst.src2, src2IsAddr);
                for (auto& arg : inst.args) prop(arg, false);
                for (auto& pair : inst.phiIncoming) prop(pair.first, false);

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
                    // A STORE to a slot vreg assigns that variable
                    if ((inst.op == Op::STORE || inst.op == Op::STORE_ZP) && inst.src2.isVreg() &&
                        slots.count(inst.src2.vregId))
                        invalidate(inst.src2.vregId);
                    else
                        invalidateVariables();
                }

                // The instruction (re)defines its destination
                if (inst.dest.isVreg()) invalidate(inst.dest.vregId);

                // 3. Record new copies directly
                if (inst.op == Op::COPY) {
                    if (inst.dest.isVreg() && !slots.count(inst.dest.vregId) &&
                        !(inst.src1.isVreg() && inst.src1.vregId == inst.dest.vregId))
                        copies[inst.dest.vregId] = inst.src1;
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
                            trace("cse", fn.name, "v" + std::to_string(inst.dest.vregId) + " reuses v" + std::to_string(pair.second.vregId), inst.loc);
                            inst.op = Op::COPY;
                            inst.src1 = pair.second;
                            inst.src2 = Operand::none();
                            if (!slots.count(inst.dest.vregId)) copies[inst.dest.vregId] = pair.second;
                            // std::printf("    Matched CSE! Replace with copy of %d\n", pair.second.vregId);
                            found = true;
                            break;
                        }
                    }

                    // A slot vreg's value changes with each assignment: only a
                    // temp can stand for the expression
                    if (!found && !slots.count(inst.dest.vregId) &&
                        !uses(inst.src1, inst.dest.vregId) && !uses(inst.src2, inst.dest.vregId)) {
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
        if (!runOn(fn, "StrengthReduction")) continue;
        // Build constant value map: vreg → immediate value
        // (for looking up CONST operands of MUL/DIV/MOD)
        std::map<uint32_t, int64_t> constVals = singleDefConsts(fn);

        // Rewrite instructions
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                // x / x → 1, x % x → 0 (when both operands are the same vreg)
                if (inst.src1.isVreg() && inst.src2.isVreg() &&
                    inst.src1.vregId == inst.src2.vregId) {
                    if (inst.op == Op::DIV || inst.op == Op::DIV_U) {
                        trace("strength", fn.name, "v" + std::to_string(inst.dest.vregId) + " = v/v → 1", inst.loc);
                        inst.op = Op::CONST;
                        inst.src1 = Operand::imm(1, inst.resultType);
                        inst.src2 = Operand::none();
                        continue;
                    }
                    if (inst.op == Op::MOD || inst.op == Op::MOD_U) {
                        trace("strength", fn.name, "v" + std::to_string(inst.dest.vregId) + " = v%v → 0", inst.loc);
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

                auto vd = [&]() { return "v" + std::to_string(inst.dest.vregId); };
                switch (inst.op) {
                    case Op::MUL:
                    case Op::MUL_U: {
                        if (val == 0) {
                            trace("strength", fn.name, vd() + " = MUL *0 → CONST 0", inst.loc);
                            inst.op = Op::CONST;
                            inst.src1 = Operand::imm(0, inst.resultType);
                            inst.src2 = Operand::none();
                        } else if (val == 1) {
                            trace("strength", fn.name, vd() + " = MUL *1 → COPY", inst.loc);
                            inst.op = Op::COPY;
                            inst.src2 = Operand::none();
                        } else if (val > 0 && (val & (val - 1)) == 0) {
                            int shift = 0;
                            int64_t v = val;
                            while (v > 1) { v >>= 1; shift++; }
                            trace("strength", fn.name, vd() + " = MUL *" + std::to_string(val) + " → SHL " + std::to_string(shift), inst.loc);
                            inst.op = Op::SHL;
                            inst.src2 = Operand::imm(shift, inst.resultType);
                        }
                        break;
                    }
                    case Op::DIV:
                    case Op::DIV_U: {
                        if (val == 1) {
                            trace("strength", fn.name, vd() + " = DIV /1 → COPY", inst.loc);
                            inst.op = Op::COPY;
                            inst.src2 = Operand::none();
                        } else if (inst.op == Op::DIV_U && val > 0 && (val & (val - 1)) == 0) {
                            int shift = 0;
                            int64_t v = val;
                            while (v > 1) { v >>= 1; shift++; }
                            trace("strength", fn.name, vd() + " = DIV_U /" + std::to_string(val) + " → SHR " + std::to_string(shift), inst.loc);
                            inst.op = Op::SHR;
                            inst.src2 = Operand::imm(shift, inst.resultType);
                        }
                        break;
                    }
                    case Op::MOD:
                    case Op::MOD_U: {
                        if (val == 1) {
                            trace("strength", fn.name, vd() + " = MOD %1 → CONST 0", inst.loc);
                            inst.op = Op::CONST;
                            inst.src1 = Operand::imm(0, inst.resultType);
                            inst.src2 = Operand::none();
                        } else if (inst.op == Op::MOD_U && val > 0 && (val & (val - 1)) == 0) {
                            trace("strength", fn.name, vd() + " = MOD_U %" + std::to_string(val) + " → AND " + std::to_string(val - 1), inst.loc);
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
        if (!runOn(fn, "AlgebraicSimplify")) continue;
        // Build constant value map
        std::map<uint32_t, int64_t> constVals = singleDefConsts(fn);

        // An element address with a constant index (after loop unrolling,
        // for example) on a global or pointer base: base + index * stride,
        // which the codegen folds into an absolute address. (A local array's
        // slot as the base means the variable's address: left alone.)
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.op != Op::ADDR_ELEM || inst.args.empty() || !inst.args[0].isImm()) continue;
                int64_t idx;
                if (inst.src2.isImm()) idx = inst.src2.immVal;
                else if (inst.src2.isVreg() && constVals.count(inst.src2.vregId)) idx = constVals[inst.src2.vregId];
                else continue;
                bool baseOk = inst.src1.kind == OperandKind::GLOBAL ||
                    (inst.src1.isVreg() && !fn.localSlotVregs.count(inst.src1.vregId) &&
                     !fn.memoryVregs.count(inst.src1.vregId));
                if (!baseOk) continue;
                int64_t off = idx * inst.args[0].immVal;
                trace("algebraic", fn.name, "v" + std::to_string(inst.dest.vregId) +
                      " = ADDR_ELEM [const " + std::to_string(idx) + "] → ADD +" + std::to_string(off), inst.loc);
                inst.op = Op::ADD;
                inst.resultType = Type::PTR;
                inst.src2 = Operand::imm(off & 0xFFFF, Type::I16);
                inst.args.clear();
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
                            trace("algebraic", fn.name, "v" + std::to_string(inst.dest.vregId) + " = x^x/x-x → 0", inst.loc);
                            inst.op = Op::CONST;
                            inst.src1 = Operand::imm(0, inst.resultType);
                            inst.src2 = Operand::none();
                            continue;
                        case Op::AND:   // x & x → x
                        case Op::OR:    // x | x → x
                            trace("algebraic", fn.name, "v" + std::to_string(inst.dest.vregId) + " = x&x/x|x → COPY", inst.loc);
                            inst.op = Op::COPY;
                            inst.src2 = Operand::none();
                            continue;
                        default: break;
                    }
                }

                // Constant-operand patterns: check src2
                int64_t val2;
                if (getConst(inst.src2, val2)) {
                    auto vd = [&]() { return "v" + std::to_string(inst.dest.vregId); };
                    switch (inst.op) {
                        case Op::ADD:
                        case Op::SUB:
                            if (val2 == 0) {
                                // x + 0 → x, x - 0 → x
                                trace("algebraic", fn.name, vd() + " = ADD/SUB +0 → COPY", inst.loc);
                                inst.op = Op::COPY;
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::AND:
                            if (val2 == 0) {
                                // x & 0 → 0
                                trace("algebraic", fn.name, vd() + " = AND &0 → CONST 0", inst.loc);
                                inst.op = Op::CONST;
                                inst.src1 = Operand::imm(0, inst.resultType);
                                inst.src2 = Operand::none();
                            } else if (val2 == fullMask(inst.resultType)) {
                                // x & 0xFFFF (I16) → x
                                trace("algebraic", fn.name, vd() + " = AND &mask → COPY", inst.loc);
                                inst.op = Op::COPY;
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::OR:
                            if (val2 == 0) {
                                // x | 0 → x
                                trace("algebraic", fn.name, vd() + " = OR |0 → COPY", inst.loc);
                                inst.op = Op::COPY;
                                inst.src2 = Operand::none();
                            } else if (val2 == fullMask(inst.resultType)) {
                                // x | 0xFFFF → 0xFFFF
                                trace("algebraic", fn.name, vd() + " = OR |mask → CONST mask", inst.loc);
                                inst.op = Op::CONST;
                                inst.src1 = Operand::imm(val2, inst.resultType);
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::XOR:
                            if (val2 == 0) {
                                // x ^ 0 → x
                                trace("algebraic", fn.name, vd() + " = XOR ^0 → COPY", inst.loc);
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
                                trace("algebraic", fn.name, vd() + " = shift <<0 → COPY", inst.loc);
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
                    auto vd = [&]() { return "v" + std::to_string(inst.dest.vregId); };
                    switch (inst.op) {
                        case Op::ADD:
                            if (val1 == 0) {
                                // 0 + x → x
                                trace("algebraic", fn.name, vd() + " = 0+x → COPY", inst.loc);
                                inst.op = Op::COPY;
                                inst.src1 = inst.src2;
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::AND:
                            if (val1 == 0) {
                                // 0 & x → 0
                                trace("algebraic", fn.name, vd() + " = 0&x → CONST 0", inst.loc);
                                inst.op = Op::CONST;
                                inst.src1 = Operand::imm(0, inst.resultType);
                                inst.src2 = Operand::none();
                            }
                            break;
                        case Op::OR:
                            if (val1 == 0) {
                                // 0 | x → x
                                trace("algebraic", fn.name, vd() + " = 0|x → COPY", inst.loc);
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
        if (!runOn(fn, "AddrElemFusion")) continue;
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
                // A base that is a local's slot vreg is the variable's address in
                // ADDR_ELEM (an array or vector local); the fused access would
                // read the variable's contents as a pointer instead
                if (addrInst.src1.isVreg() && (fn.localSlotVregs.count(addrInst.src1.vregId) ||
                                               fn.memoryVregs.count(addrInst.src1.vregId))) continue;

                // Next instruction must be LOAD or STORE using this vreg as address
                bool isLoad = (nextInst.op == Op::LOAD && nextInst.src1.isVreg() &&
                               nextInst.src1.vregId == addrVreg);
                bool isStore = (nextInst.op == Op::STORE && nextInst.src2.isVreg() &&
                                nextInst.src2.vregId == addrVreg);
                if (!isLoad && !isStore) continue;
                // The codegen computes a fused address only for 1/2/4-byte
                // integer and pointer accesses (a float or I_N access keeps
                // its address vreg)
                // (and for loads only: the fused 4-byte store is not implemented)
                Type t = nextInst.resultType;
                if (t != Type::I8 && t != Type::I16 && t != Type::PTR && !(isLoad && t == Type::I32)) continue;
                if (!nextInst.args.empty()) continue;   // already carries other operands

                // Fuse: copy ADDR_ELEM params into the LOAD/STORE args
                trace("fusion", fn.name, "v" + std::to_string(addrVreg) + " ADDR_ELEM fused into " + (isLoad ? "LOAD" : "STORE"), addrInst.loc);
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
        if (!runOn(fn, "CopyPropagation")) continue;
        // Build forwarding map: follow COPY chains to the root operand.
        // %a = COPY %b; %c = COPY %a → %c should reference %b directly.
        // The map is used across the whole function, so (vregs not being SSA)
        // only a copy defined once whose root is a temp defined at most once
        // — never a variable's slot vreg, which assignments change — is
        // forwarded.
        std::map<uint32_t, int> defCount;
        for (auto& block : fn.blocks)
            for (auto& inst : block.insts) {
                if (inst.dest.isVreg()) defCount[inst.dest.vregId]++;
                if ((inst.op == Op::STORE || inst.op == Op::STORE_ZP) && inst.src2.isVreg())
                    defCount[inst.src2.vregId] += 2;   // assigned: never a fixed value
            }
        auto fixedTemp = [&](uint32_t v) {
            return !fn.localSlotVregs.count(v) && defCount[v] <= 1;
        };
        std::map<uint32_t, Operand> copyRoot;
        for (auto& block : fn.blocks) {
            for (auto& inst : block.insts) {
                if (inst.op == Op::COPY && inst.dest.isVreg() && inst.src1.isVreg() &&
                    fixedTemp(inst.dest.vregId) && fixedTemp(inst.src1.vregId)) {
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

        if (copyRoot.empty()) continue;

        for (auto& [dest, root] : copyRoot) {
            trace("copy", fn.name, "v" + std::to_string(dest) + " chain → v" + std::to_string(root.vregId));
        }

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
                    trace("copy", fn.name, "v" + std::to_string(inst.dest.vregId) + " dead COPY removed", inst.loc);
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

static void forwardNarrowing(Function& fn);

void optimizeTypeNarrowing(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "TypeNarrowing")) continue;
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
                case Op::SHL:   // not SHR: the low byte of x >> n comes from x's high bits
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
                trace("narrow", fn.name, "v" + std::to_string(inst.dest.vregId) + " narrowed to I8", inst.loc);
                inst.resultType = Type::I8;

                // Convert each TRUNC consumer to COPY (the result is already I8)
                auto& useList = uses[inst.dest.vregId];
                for (auto& u : useList) {
                    if (u.inst->op == Op::TRUNC && u.inst->resultType == Type::I8) {
                        u.inst->op = Op::COPY;
                        u.inst->resultType = Type::I8;
                        // the operand is an I8 value now (copy propagation
                        // passes this operand on to the copy's users)
                        u.inst->src1.type = Type::I8;
                    }
                }
            }
        }

        // Pass 1b: operations on zero-extended bytes on the bytes; constant
        // operands as immediates
        forwardNarrowing(fn);

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
                trace("narrow", fn.name, "v" + std::to_string(inst.dest.vregId) + " redundant ZEXT/SEXT → COPY", inst.loc);
                inst.op = Op::COPY;
                inst.resultType = Type::I8;

                // Convert TRUNC consumers to COPY
                auto& useList = uses[inst.dest.vregId];
                for (auto& u : useList) {
                    if (u.inst->op == Op::TRUNC && u.inst->resultType == Type::I8) {
                        u.inst->op = Op::COPY;
                        u.inst->resultType = Type::I8;
                        // the operand is an I8 value now (copy propagation
                        // passes this operand on to the copy's users)
                        u.inst->src1.type = Type::I8;
                    }
                }
            }
        }
    }
}

// Forward narrowing and constant operands (part of TypeNarrowing).
//
// C promotes a byte to int before & | ^ and comparisons, so `flags & 1` or
// `c == 'a'` arrive as 16-bit operations on zero-extended bytes, with the
// constant in a vreg of its own. Here:
// - a ZEXT of a constant becomes a wider constant;
// - a vreg defined by one CONST becomes an immediate second operand of the
//   ops IRCodeGen takes immediates for (commutative ops with a constant first
//   operand are swapped);
// - AND/OR/XOR of zero-extended bytes (or a byte and a constant) are computed
//   on the bytes and zero-extended once: %t = op i8 a, b ; %d = zext %t;
// - comparisons of zero-extended bytes (and constants 0..255) compare the
//   bytes (unsigned: zero-extended values are never negative);
// - a branch on a zero-extended byte tests the byte.
// A vreg counts as fixed only with exactly one definition and no STORE to it
// (the IR is not SSA).
static void forwardNarrowing(Function& fn) {
    std::map<uint32_t, int> defCount;
    std::set<uint32_t> storedTo;
    for (auto& block : fn.blocks)
        for (auto& inst : block.insts) {
            if (inst.dest.isVreg()) defCount[inst.dest.vregId]++;
            if ((inst.op == Op::STORE || inst.op == Op::STORE_ZP) && inst.src2.isVreg())
                storedTo.insert(inst.src2.vregId);
        }
    auto single = [&](uint32_t v) {
        auto it = defCount.find(v);
        return it != defCount.end() && it->second == 1 && !storedTo.count(v) && !fn.localSlotVregs.count(v);
    };
    // the one definition of each single-def vreg
    std::map<uint32_t, Inst*> def;
    for (auto& block : fn.blocks)
        for (auto& inst : block.insts)
            if (inst.dest.isVreg() && single(inst.dest.vregId)) def[inst.dest.vregId] = &inst;
    auto constOf = [&](const Operand& o, int64_t& v) -> bool {
        if (o.isImm()) { v = o.immVal; return true; }
        if (!o.isVreg()) return false;
        auto it = def.find(o.vregId);
        if (it == def.end() || it->second->op != Op::CONST || !it->second->src1.isImm()) return false;
        v = it->second->src1.immVal;
        return true;
    };
    // a zero-extended byte: the I8 source vreg
    auto zextByte = [&](const Operand& o, Operand& src) -> bool {
        if (!o.isVreg()) return false;
        auto it = def.find(o.vregId);
        if (it == def.end() || it->second->op != Op::ZEXT) return false;
        const Operand& s1 = it->second->src1;
        if (!s1.isVreg() || s1.type != Type::I8) return false;
        src = s1;
        return true;
    };
    auto isInt = [](Type t) { return t == Type::I8 || t == Type::I16 || t == Type::I32; };

    // 1. ZEXT of a constant: a wider constant
    for (auto& block : fn.blocks)
        for (auto& inst : block.insts) {
            int64_t v;
            if (inst.op == Op::ZEXT && inst.dest.isVreg() && isInt(inst.resultType) && constOf(inst.src1, v) &&
                (inst.src1.type == Type::I8 || inst.src1.type == Type::I16)) {
                inst.op = Op::CONST;
                inst.src1 = Operand::imm(v & (inst.src1.type == Type::I8 ? 0xFF : 0xFFFF), inst.resultType);
            }
        }

    // 2. narrowing of AND/OR/XOR, comparisons, branches
    for (auto& block : fn.blocks) {
        for (size_t i = 0; i < block.insts.size(); i++) {
            Inst& inst = block.insts[i];
            bool bitwise = inst.op == Op::AND || inst.op == Op::OR || inst.op == Op::XOR;
            bool cmp = inst.op >= Op::CMP_EQ && inst.op <= Op::CMP_GEU;
            if (inst.op == Op::BR_COND) {
                Operand b;
                if (zextByte(inst.src1, b)) inst.src1 = b;
                continue;
            }
            if (!bitwise && !cmp) continue;
            if (inst.src1.type != Type::I16 && inst.src1.type != Type::I32) continue;
            Operand a, b;
            int64_t k;
            bool aByte = zextByte(inst.src1, a);
            bool bByte = zextByte(inst.src2, b);
            bool bConst = constOf(inst.src2, k);
            if (!aByte) continue;
            if (!bByte) {
                if (!bConst) continue;
                if (inst.op == Op::AND) k &= 0xFF;          // the high bits of a are 0
                else if (k < 0 || k > 0xFF) continue;
                b = Operand::imm(k, Type::I8);
            }
            if (cmp) {
                // zero-extended values compare as unsigned bytes
                switch (inst.op) {
                    case Op::CMP_LT: inst.op = Op::CMP_LTU; break;
                    case Op::CMP_LE: inst.op = Op::CMP_LEU; break;
                    case Op::CMP_GT: inst.op = Op::CMP_GTU; break;
                    case Op::CMP_GE: inst.op = Op::CMP_GEU; break;
                    default: break;
                }
                inst.src1 = a;
                inst.src2 = b;
                trace("narrow", fn.name, "compare of zero-extended bytes on the bytes", inst.loc);
                continue;
            }
            if (!inst.dest.isVreg() || fn.localSlotVregs.count(inst.dest.vregId)) continue;
            if (inst.resultType == Type::I8) {
                // the result is a byte already: only the operands change
                inst.src1 = a;
                inst.src2 = b;
                continue;
            }
            // %t = op i8 a, b ; %d = zext %t
            uint32_t t = fn.allocVreg();
            Inst narrow = inst;
            narrow.dest = Operand::vreg(t, Type::I8);
            narrow.resultType = Type::I8;
            narrow.src1 = a;
            narrow.src2 = b;
            Inst ext;
            ext.op = Op::ZEXT;
            ext.dest = inst.dest;
            ext.resultType = inst.resultType;
            ext.src1 = Operand::vreg(t, Type::I8);
            ext.loc = inst.loc;
            trace("narrow", fn.name, "v" + std::to_string(inst.dest.vregId) + " computed on bytes", inst.loc);
            block.insts[i] = ext;
            block.insts.insert(block.insts.begin() + i, narrow);
            i++;
        }
    }

    // 3. constant second operands as immediates
    for (auto& block : fn.blocks) {
        for (auto& inst : block.insts) {
            bool commutative = inst.op == Op::ADD || inst.op == Op::AND || inst.op == Op::OR ||
                               inst.op == Op::XOR || inst.op == Op::CMP_EQ || inst.op == Op::CMP_NE;
            bool takesImm = commutative || inst.op == Op::SUB || inst.op == Op::SHL || inst.op == Op::SHR ||
                            inst.op == Op::ASR || (inst.op >= Op::CMP_EQ && inst.op <= Op::CMP_GEU);
            if (!takesImm || (!isInt(inst.src1.type) && inst.src1.type != Type::PTR)) continue;
            int64_t v;
            if (commutative && !inst.src2.isImm() && constOf(inst.src1, v) && inst.src2.isVreg())
                std::swap(inst.src1, inst.src2);
            if (inst.src2.isVreg() && constOf(inst.src2, v)) {
                Type t = inst.src2.type;
                inst.src2 = Operand::imm(v, t);
            }
        }
    }

    // 4. unsigned comparisons with 0 or 1: an unsigned value is never below 0,
    // so x > 0 is x != 0 (the Z flag alone, no carry), x >= 0 is true, ...
    for (auto& block : fn.blocks) {
        for (auto& inst : block.insts) {
            if (inst.op != Op::CMP_LTU && inst.op != Op::CMP_LEU &&
                inst.op != Op::CMP_GTU && inst.op != Op::CMP_GEU) continue;
            if (!inst.dest.isVreg() || fn.localSlotVregs.count(inst.dest.vregId)) continue;
            int64_t v;
            Op op = inst.op;
            Operand x;
            if (constOf(inst.src2, v) && !constOf(inst.src1, v)) {
                constOf(inst.src2, v);
                x = inst.src1;
            } else if (constOf(inst.src1, v) && !constOf(inst.src2, v)) {
                constOf(inst.src1, v);
                x = inst.src2;
                // k OP x  ->  x OP' k
                op = op == Op::CMP_LTU ? Op::CMP_GTU : op == Op::CMP_LEU ? Op::CMP_GEU :
                     op == Op::CMP_GTU ? Op::CMP_LTU : Op::CMP_LEU;
            } else continue;
            int result = -1;          // 0 / 1: always false / true
            Op eq = Op::NOP;          // or x == 0 / x != 0
            if (v == 0) {
                if (op == Op::CMP_GTU) eq = Op::CMP_NE;
                else if (op == Op::CMP_LEU) eq = Op::CMP_EQ;
                else if (op == Op::CMP_GEU) result = 1;
                else result = 0;
            } else if (v == 1) {
                if (op == Op::CMP_LTU) eq = Op::CMP_EQ;
                else if (op == Op::CMP_GEU) eq = Op::CMP_NE;
                else continue;
            } else continue;
            if (result >= 0) {
                inst.op = Op::CONST;
                inst.src1 = Operand::imm(result, inst.resultType);
                inst.src2 = Operand::none();
                trace("narrow", fn.name, "unsigned compare with 0 is constant", inst.loc);
            } else {
                inst.op = eq;
                inst.src1 = x;
                inst.src2 = Operand::imm(0, x.type);
                trace("narrow", fn.name, "unsigned compare with 0/1 is a test for 0", inst.loc);
            }
        }
    }
}

// ============================================================================
// Branch Folding and Dead Block Elimination
// ============================================================================

void optimizeBranchFold(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "BranchFolding")) continue;
        bool changed = true;
        int maxIter = 16;
        while (changed && --maxIter > 0) {
            changed = false;

            // Build constant value map. Vregs are not SSA: a vreg is a constant
            // only when its one definition is a CONST and nothing stores to it
            // (a && b's result gets CONST 0 on one path and a value on the other;
            // a STORE to a local's slot vreg assigns the variable)
            std::map<uint32_t, int64_t> constVals;
            std::map<uint32_t, int> defCount;
            std::set<uint32_t> storedTo;
            for (auto& block : fn.blocks) {
                for (auto& inst : block.insts) {
                    if (inst.dest.isVreg()) defCount[inst.dest.vregId]++;
                    if ((inst.op == Op::STORE || inst.op == Op::STORE_ZP) && inst.src2.isVreg())
                        storedTo.insert(inst.src2.vregId);
                    if (inst.op == Op::CONST && inst.dest.isVreg() && inst.src1.isImm())
                        constVals[inst.dest.vregId] = inst.src1.immVal;
                }
            }
            for (auto it = constVals.begin(); it != constVals.end();) {
                if (defCount[it->first] != 1 || storedTo.count(it->first) || fn.localSlotVregs.count(it->first))
                    it = constVals.erase(it);
                else
                    ++it;
            }

            // Pass 0: instructions after a block's first terminator never run
            // (break inside if: "br @while_end; br @if_end"); later passes and
            // the codegen treat the last instruction as the terminator
            for (auto& blk : fn.blocks) {
                for (size_t k = 0; k + 1 < blk.insts.size(); k++) {
                    Op op = blk.insts[k].op;
                    if (op == Op::BR || op == Op::BR_COND || op == Op::BR_INDIRECT || op == Op::SWITCH ||
                        op == Op::RET || op == Op::RET_VOID) {
                        trace("branch", fn.name, "unreachable instructions after the terminator of " + blk.label + " removed");
                        blk.insts.erase(blk.insts.begin() + k + 1, blk.insts.end());
                        changed = true;
                        break;
                    }
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
                        trace("branch", fn.name, "BR_COND const → fallthrough");
                        fn.blocks[bi].insts.erase(it);
                    } else {
                        trace("branch", fn.name, "BR_COND const → BR " + target);
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
                                    trace("branch", fn.name, "thread " + label + " → " + target.insts[0].src1.name);
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

            // Pass 2a: a conditional branch on a constant (if (0) { ... } kept
            // for a case label inside it) becomes a plain branch
            for (auto& blk : fn.blocks) {
                if (blk.insts.empty() || blk.insts.back().op != Op::BR_COND) continue;
                auto& br = blk.insts.back();
                bool known = false;
                int64_t v = 0;
                if (br.src1.isImm()) { known = true; v = br.src1.immVal; }
                else if (br.src1.isVreg()) {
                    for (size_t k = blk.insts.size() - 1; k-- > 0;) {
                        const auto& d = blk.insts[k];
                        if (d.dest.isVreg() && d.dest.vregId == br.src1.vregId) {
                            if (d.op == Op::CONST && d.src1.isImm()) { known = true; v = d.src1.immVal; }
                            break;
                        }
                    }
                }
                if (!known || br.dest.name.empty() || br.src2.name.empty()) continue;
                std::string target = v ? br.dest.name : br.src2.name;
                trace("branch", fn.name, "constant condition: br " + target, br.loc);
                Inst b;
                b.op = Op::BR;
                b.src1 = Operand::label(target);
                b.loc = br.loc;
                br = b;
                changed = true;
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
                // A label whose address is taken can be reached by goto *
                referenced.insert(fn.addressTakenLabels.begin(), fn.addressTakenLabels.end());

                for (size_t i = 0; i < fn.blocks.size(); i++) {
                    auto& block = fn.blocks[i];
                    bool hasTerminator = false;
                    for (auto& inst : block.insts) {
                        if (inst.op == Op::ADDR_LABEL) referenced.insert(inst.src1.name);
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
                        trace("branch", fn.name, "dead block " + it->label + " removed");
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
        if (!runOn(fn, "LICM")) continue;
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
            bool loopWritesMemory = false;   // a call, asm or store through a pointer
            for (size_t bi : loopBlocks) {
                for (const auto& inst : fn.blocks[bi].insts) {
                    if (inst.dest.isVreg()) {
                        loopDefinedVregs.insert(inst.dest.vregId);
                    }
                    // STORE to a local slot vreg mutates that variable
                    if ((inst.op == Op::STORE || inst.op == Op::STORE_ZP) &&
                        inst.src2.isVreg() && fn.localSlotVregs.count(inst.src2.vregId)) {
                        loopMutatedSlots.insert(inst.src2.vregId);
                    } else if (inst.op == Op::STORE || inst.op == Op::STORE_ZP ||
                               inst.op == Op::CALL || inst.op == Op::CALL_VOID ||
                               inst.op == Op::CALL_INDIRECT || inst.op == Op::ASM_INLINE ||
                               inst.op == Op::CPU_REG_WRITE) {
                        loopWritesMemory = true;
                    }
                }
            }
            // A variable whose address is taken changes through such a write
            if (loopWritesMemory) {
                for (const auto& blk : fn.blocks)
                    for (const auto& inst : blk.insts)
                        if (inst.op == Op::ADDR_LOCAL && inst.src1.isVreg())
                            loopMutatedSlots.insert(inst.src1.vregId);
                for (uint32_t v : fn.memoryVregs) loopMutatedSlots.insert(v);
            }
            // Vregs are not SSA: only an instruction that is its destination's
            // one definition can move
            std::map<uint32_t, int> defCount;
            for (const auto& blk : fn.blocks)
                for (const auto& inst : blk.insts)
                    if (inst.dest.isVreg()) defCount[inst.dest.vregId]++;

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
                        if (defCount[inst.dest.vregId] != 1) continue;

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

            for (uint32_t v : nonConstInvariants) {
                trace("licm", fn.name, "v" + std::to_string(v) + " hoisted to pre-header");
            }

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


// ============================================================================
// Loop unrolling (-floop-unrolling)
// ============================================================================
//
// Fully unrolls a counted for loop when the unrolled code is estimated to fit
// unrollMaxBytes. IRBuilder lowers `for (init; cond; inc) body` to
//
//   <pred>:        ... store <const>, %v ; br @for_condN
//   @for_condN:    <v compared with a constant> ; br_cond %c, @for_bodyN, @for_endN
//   @for_bodyN ... (the body's blocks)
//   @for_incN:     %x = deref %v ; %y = add/sub %x, <const> ; store %y, %v ; br @for_condN
//   @for_endN:
//
// with %v the loop variable's slot. The trip count is simulated from the
// constants. Each copy of the body gets its own labels and reads the
// variable's value for that iteration from a fresh CONST temp (so the other
// passes can fold indexes and conditions with it); `continue` jumps to the
// next copy, `break` to @for_endN. The variable gets its final value after
// the last copy (and, when the body can break out, at the start of each copy).
//
// Not unrolled: a body that assigns the variable, takes its address, contains
// asm, address-taken labels or labels reached from outside the loop; a
// variable in memory (volatile, address-taken); values that would wrap or
// cross the sign bit; more than 255 iterations.

int unrollMaxBytes = 128;

namespace {

struct ForLoop {
    size_t cond, body, inc, end;   // block indexes
    uint32_t var;                  // the loop variable's slot vreg
    std::vector<int64_t> values;   // the variable's value in each iteration
    int64_t finalValue;
    Type varType;
};

bool isConstOperand(const Function& fn, const Operand& o, int64_t& v,
                    const std::map<uint32_t, int>& defCount) {
    if (o.isImm()) { v = o.immVal; return true; }
    if (!o.isVreg()) return false;
    auto dc = defCount.find(o.vregId);
    if (dc == defCount.end() || dc->second != 1) return false;
    for (const auto& b : fn.blocks)
        for (const auto& i : b.insts)
            if (i.dest.isVreg() && i.dest.vregId == o.vregId) {
                if (i.op == Op::CONST && i.src1.isImm()) { v = i.src1.immVal; return true; }
                // an extension of a constant (the callers only accept small
                // non-negative bounds, for which zext/sext/copy keep the value)
                if (i.op == Op::ZEXT || i.op == Op::SEXT || i.op == Op::COPY)
                    return isConstOperand(fn, i.src1, v, defCount);
                return false;
            }
    return false;
}

bool matchForLoop(const Function& fn, size_t condIdx, const std::map<uint32_t, int>& defCount, ForLoop& L) {
    const std::string& cl = fn.blocks[condIdx].label;
    if (cl.compare(0, 8, "for_cond") != 0) return false;
    // (the labels share one counter: for_cond0, for_body1, for_inc2, for_end3)
    const Block& cb = fn.blocks[condIdx];
    if (cb.insts.empty() || cb.insts.back().op != Op::BR_COND) return false;
    const Inst& br = cb.insts.back();
    L.cond = condIdx;
    L.body = L.inc = L.end = SIZE_MAX;
    for (size_t k = condIdx + 1; k < fn.blocks.size(); k++) {
        if (fn.blocks[k].label == br.dest.name && L.body == SIZE_MAX) L.body = k;
        if (fn.blocks[k].label == br.src2.name) { L.end = k; break; }
    }
    if (L.body != condIdx + 1 || L.end == SIZE_MAX || L.end < L.body + 2) return false;
    L.inc = L.end - 1;
    if (fn.blocks[L.body].label.compare(0, 8, "for_body") != 0 ||
        fn.blocks[L.inc].label.compare(0, 7, "for_inc") != 0) return false;

    // Condition block: v (through deref/copy/zext/sext) compared with a constant
    std::map<uint32_t, uint32_t> fromVar;   // vreg -> the slot it reads
    const Inst* cmp = nullptr;
    for (size_t k = 0; k + 1 < cb.insts.size(); k++) {
        const Inst& i = cb.insts[k];
        if (i.op == Op::CONST) continue;
        if ((i.op == Op::DEREF || i.op == Op::COPY || i.op == Op::ZEXT || i.op == Op::SEXT) &&
            i.src1.isVreg() && i.dest.isVreg()) {
            uint32_t s = i.src1.vregId;
            int64_t cv;
            if (fn.localSlotVregs.count(s)) fromVar[i.dest.vregId] = s;
            else if (fromVar.count(s)) fromVar[i.dest.vregId] = fromVar[s];
            else if (!isConstOperand(fn, i.src1, cv, defCount)) return false;   // (zext of the bound)
            continue;
        }
        if (i.op >= Op::CMP_EQ && i.op <= Op::CMP_GEU && i.dest.isVreg() && br.src1.isVreg() &&
            i.dest.vregId == br.src1.vregId && !cmp) { cmp = &i; continue; }
        return false;
    }
    if (!cmp) return false;
    int64_t bound;
    uint32_t var;
    if (cmp->src1.isVreg() && fn.localSlotVregs.count(cmp->src1.vregId)) var = cmp->src1.vregId;
    else if (cmp->src1.isVreg() && fromVar.count(cmp->src1.vregId)) var = fromVar[cmp->src1.vregId];
    else return false;
    if (!isConstOperand(fn, cmp->src2, bound, defCount)) return false;
    if (fn.memoryVregs.count(var)) return false;
    L.var = var;

    // Increment block: v = v +/- step
    const Block& ib = fn.blocks[L.inc];
    if (ib.insts.empty() || ib.insts.back().op != Op::BR || ib.insts.back().src1.name != cl) return false;
    std::map<uint32_t, int64_t> varPlus;    // vreg -> v + offset
    int64_t step = 0;
    bool stored = false;
    for (size_t k = 0; k + 1 < ib.insts.size(); k++) {
        const Inst& i = ib.insts[k];
        if (i.op == Op::CONST) continue;
        if ((i.op == Op::DEREF || i.op == Op::COPY) && i.src1.isVreg() && i.src1.vregId == var && i.dest.isVreg()) {
            varPlus[i.dest.vregId] = 0; continue;
        }
        // widening/narrowing of v + offset (c++ on a char): the values are
        // checked below to stay in range, so the offset is unchanged
        if ((i.op == Op::ZEXT || i.op == Op::SEXT || i.op == Op::TRUNC || i.op == Op::COPY) &&
            i.src1.isVreg() && varPlus.count(i.src1.vregId) && i.dest.isVreg()) {
            varPlus[i.dest.vregId] = varPlus[i.src1.vregId]; continue;
        }
        if ((i.op == Op::ADD || i.op == Op::SUB) && i.dest.isVreg()) {
            int64_t c;
            int64_t base;
            if (i.src1.isVreg() && i.src1.vregId == var) base = 0;
            else if (i.src1.isVreg() && varPlus.count(i.src1.vregId)) base = varPlus[i.src1.vregId];
            else return false;
            if (!isConstOperand(fn, i.src2, c, defCount)) return false;
            varPlus[i.dest.vregId] = base + (i.op == Op::ADD ? c : -c);
            continue;
        }
        if (i.op == Op::STORE && i.src2.isVreg() && i.src2.vregId == var && i.src1.isVreg() &&
            varPlus.count(i.src1.vregId) && !stored) {
            step = varPlus[i.src1.vregId]; stored = true; continue;
        }
        return false;
    }
    if (!stored || step == 0) return false;

    // Initial value: the block before the condition stores a constant into v
    if (condIdx == 0) return false;
    const Block& pb = fn.blocks[condIdx - 1];
    if (pb.insts.empty() || pb.insts.back().op != Op::BR || pb.insts.back().src1.name != cl) return false;
    int64_t init = 0;
    bool haveInit = false;
    for (size_t k = pb.insts.size() - 1; k-- > 0;) {
        const Inst& i = pb.insts[k];
        if (i.op == Op::STORE && i.src2.isVreg() && i.src2.vregId == var) {
            haveInit = isConstOperand(fn, i.src1, init, defCount);
            break;
        }
        if (i.op == Op::CALL || i.op == Op::CALL_VOID || i.op == Op::CALL_INDIRECT || i.op == Op::ASM_INLINE) break;
    }
    if (!haveInit) return false;
    // The only other edges into the condition: the increment (and the pred)
    for (size_t b = 0; b < fn.blocks.size(); b++) {
        if (b == condIdx - 1 || b == L.inc) continue;
        for (const auto& i : fn.blocks[b].insts) {
            if ((i.op == Op::BR && i.src1.name == cl) ||
                (i.op == Op::BR_COND && (i.dest.name == cl || i.src2.name == cl))) return false;
        }
    }

    // Trip count: every value non-negative and below the sign bit of its type
    auto vt = fn.vregTypes.find(var);
    L.varType = vt != fn.vregTypes.end() ? vt->second : Type::I16;
    int bits = L.varType == Type::I8 ? 8 : L.varType == Type::I32 ? 32 : 16;
    int64_t limit = (int64_t)1 << (bits - 1);
    if (bound < 0 || bound >= limit) return false;
    int64_t v = init;
    L.values.clear();
    for (;;) {
        if (v < -limit || v >= limit) return false;
        bool go;
        switch (cmp->op) {
            case Op::CMP_LT: case Op::CMP_LTU: go = v < bound; break;
            case Op::CMP_LE: case Op::CMP_LEU: go = v <= bound; break;
            case Op::CMP_GT: case Op::CMP_GTU: go = v > bound; break;
            case Op::CMP_GE: case Op::CMP_GEU: go = v >= bound; break;
            case Op::CMP_NE: go = v != bound; break;
            case Op::CMP_EQ: go = v == bound; break;
            default: return false;
        }
        if (!go) break;
        if (v < 0) return false;                 // a negative value of a (maybe) unsigned variable
        L.values.push_back(v);
        if (L.values.size() > 255) return false;
        v += step;
    }
    // An unsigned comparison of a final value below zero would have wrapped
    if (v < 0 && cmp->op >= Op::CMP_LTU) return false;
    L.finalValue = v;
    return true;
}

} // namespace

void optimizeLoopUnroll(Module& mod) {
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "LoopUnrolling")) continue;
        bool changed = true;
        int guard = 64;
        while (changed && --guard > 0) {
            changed = false;
            std::map<uint32_t, int> defCount;
            for (const auto& b : fn.blocks)
                for (const auto& i : b.insts) {
                    if (i.dest.isVreg()) defCount[i.dest.vregId]++;
                    if ((i.op == Op::STORE || i.op == Op::STORE_ZP) && i.src2.isVreg()) defCount[i.src2.vregId] += 2;
                }
            for (size_t c = 0; c < fn.blocks.size() && !changed; c++) {
                ForLoop L;
                if (!matchForLoop(fn, c, defCount, L)) continue;
                // Innermost first: no other loop condition inside the body
                bool inner = false;
                for (size_t b = L.body; b < L.inc; b++)
                    if (fn.blocks[b].label.compare(0, 8, "for_cond") == 0 ||
                        fn.blocks[b].label.compare(0, 10, "while_cond") == 0 ||
                        fn.blocks[b].label.compare(0, 7, "do_body") == 0) inner = true;
                if (inner) continue;

                // The body: no asm, no assignment / address of the variable,
                // labels only reached from inside the loop
                std::set<std::string> bodyLabels;
                for (size_t b = L.body; b < L.inc; b++) bodyLabels.insert(fn.blocks[b].label);
                bool ok = true, breaks = false;
                int size = 0;
                for (size_t b = L.body; b < L.inc && ok; b++) {
                    if (fn.addressTakenLabels.count(fn.blocks[b].label)) ok = false;
                    for (const auto& i : fn.blocks[b].insts) {
                        if (i.op == Op::NOP) continue;
                        size++;
                        if (i.op == Op::ASM_INLINE || i.op == Op::ADDR_LABEL || i.op == Op::BR_INDIRECT) ok = false;
                        if ((i.op == Op::STORE || i.op == Op::STORE_ZP) && i.src2.isVreg() && i.src2.vregId == L.var) ok = false;
                        if (i.op == Op::ADDR_LOCAL && i.src1.isVreg() && i.src1.vregId == L.var) ok = false;
                        if (i.dest.isVreg() && i.dest.vregId == L.var) ok = false;
                        auto target = [&](const std::string& t) {
                            if (t == fn.blocks[L.end].label) breaks = true;
                        };
                        if (i.op == Op::BR) target(i.src1.name);
                        if (i.op == Op::BR_COND) { target(i.dest.name); target(i.src2.name); }
                    }
                }
                for (size_t b = 0; b < fn.blocks.size() && ok; b++) {
                    if (b >= L.body && b < L.inc) continue;
                    for (const auto& i : fn.blocks[b].insts) {
                        auto into = [&](const std::string& t) { if (bodyLabels.count(t) && t != fn.blocks[L.body].label) ok = false; };
                        if (i.op == Op::BR) into(i.src1.name);
                        if (i.op == Op::BR_COND) { into(i.dest.name); into(i.src2.name); }
                        if (i.op == Op::SWITCH) { into(i.src2.name); for (const auto& sc : i.switchCases) into(sc.second); }
                    }
                }
                if (!ok) continue;
                // Estimate: about 4 bytes per IR instruction (and 2 per copy for a CONST/store)
                int64_t bytes = (int64_t)L.values.size() * (size * 4 + (breaks ? 6 : 2));
                if (bytes > unrollMaxBytes) continue;

                // Temporaries the body defines get fresh vregs in each copy, so
                // the unrolled code has one definition per temp, like straight
                // code (the register allocator's live ranges expect that)
                std::set<uint32_t> bodyTemps;
                for (size_t b = L.body; b < L.inc; b++)
                    for (const auto& i : fn.blocks[b].insts)
                        if (i.dest.isVreg() && !fn.localSlotVregs.count(i.dest.vregId) &&
                            !fn.memoryVregs.count(i.dest.vregId))
                            bodyTemps.insert(i.dest.vregId);
                // ... unless one is also used outside the body
                for (size_t b = 0; b < fn.blocks.size(); b++) {
                    if (b >= L.body && b < L.inc) continue;
                    for (const auto& i : fn.blocks[b].insts) {
                        auto out = [&](const Operand& o) { if (o.isVreg()) bodyTemps.erase(o.vregId); };
                        out(i.dest); out(i.src1); out(i.src2);
                        for (const auto& a : i.args) out(a);
                    }
                }

                // Build the copies
                std::string incLabel = fn.blocks[L.inc].label, endLabel = fn.blocks[L.end].label;
                std::vector<Block> out;
                size_t n = L.values.size();
                auto copyLabel = [&](const std::string& l, size_t k) { return l + "_u" + std::to_string(k); };
                for (size_t k = 0; k < n; k++) {
                    uint32_t t = fn.allocVreg();
                    fn.vregTypes[t] = L.varType;
                    std::map<uint32_t, uint32_t> fresh;
                    for (uint32_t v : bodyTemps) {
                        uint32_t nv = fn.allocVreg();
                        fresh[v] = nv;
                        auto ti = fn.vregTypes.find(v);
                        if (ti != fn.vregTypes.end()) fn.vregTypes[nv] = ti->second;
                        auto si = fn.vregSizes.find(v);
                        if (si != fn.vregSizes.end()) fn.vregSizes[nv] = si->second;
                        auto ai = fn.vregAlign.find(v);
                        if (ai != fn.vregAlign.end()) fn.vregAlign[nv] = ai->second;
                        if (fn.registerVregs.count(v)) fn.registerVregs.insert(nv);
                    }
                    auto renameTemp = [&](Operand& o) {
                        if (o.isVreg()) { auto f = fresh.find(o.vregId); if (f != fresh.end()) o.vregId = f->second; }
                    };
                    std::string next = (k + 1 < n) ? copyLabel(fn.blocks[L.body].label, k + 1) : incLabel;
                    for (size_t b = L.body; b < L.inc; b++) {
                        Block nb = fn.blocks[b];
                        nb.label = copyLabel(nb.label, k);
                        if (b == L.body) {
                            Inst ci;
                            ci.op = Op::CONST;
                            ci.dest = Operand::vreg(t, L.varType);
                            ci.src1 = Operand::imm(L.values[k], L.varType);
                            ci.resultType = L.varType;
                            std::vector<Inst> pre{ci};
                            if (breaks) {
                                Inst st;
                                st.op = Op::STORE;
                                st.resultType = L.varType;
                                st.src1 = Operand::vreg(t, L.varType);
                                st.src2 = Operand::vreg(L.var, L.varType);
                                pre.push_back(st);
                            }
                            nb.insts.insert(nb.insts.begin(), pre.begin(), pre.end());
                        }
                        auto relabel = [&](std::string& l) {
                            if (bodyLabels.count(l)) l = copyLabel(l, k);
                            else if (l == incLabel) l = next;
                        };
                        for (auto& i : nb.insts) {
                            if (i.op == Op::BR) relabel(i.src1.name);
                            if (i.op == Op::BR_COND) { relabel(i.dest.name); relabel(i.src2.name); }
                            if (i.op == Op::SWITCH) { relabel(i.src2.name); for (auto& sc : i.switchCases) relabel(sc.second); }
                            // The variable's value in this iteration
                            if (i.op == Op::CONST && i.dest.isVreg() && i.dest.vregId == t) continue;
                            if (i.op == Op::STORE && i.src2.isVreg() && i.src2.vregId == L.var) continue;
                            if ((i.op == Op::DEREF || i.op == Op::LOAD) && i.src1.isVreg() && i.src1.vregId == L.var) {
                                i.op = Op::COPY;
                                i.src1 = Operand::vreg(t, L.varType);
                                continue;
                            }
                            auto sub = [&](Operand& o) { if (o.isVreg() && o.vregId == L.var) o.vregId = t; };
                            sub(i.src1); sub(i.src2);
                            for (auto& a : i.args) sub(a);
                        }
                        for (auto& i : nb.insts) {
                            renameTemp(i.dest); renameTemp(i.src1); renameTemp(i.src2);
                            for (auto& a : i.args) renameTemp(a);
                            for (auto& ph : i.phiIncoming) renameTemp(ph.first);
                        }
                        out.push_back(std::move(nb));
                    }
                }
                // The increment block becomes: v = final value
                Block fin;
                fin.label = incLabel;
                {
                    uint32_t t = fn.allocVreg();
                    fn.vregTypes[t] = L.varType;
                    Inst ci;
                    ci.op = Op::CONST;
                    ci.dest = Operand::vreg(t, L.varType);
                    ci.src1 = Operand::imm(L.finalValue, L.varType);
                    ci.resultType = L.varType;
                    Inst st;
                    st.op = Op::STORE;
                    st.resultType = L.varType;
                    st.src1 = Operand::vreg(t, L.varType);
                    st.src2 = Operand::vreg(L.var, L.varType);
                    fin.insts = {ci, st};
                }
                out.push_back(std::move(fin));

                // The condition block jumps straight into the first copy
                Block& cb = fn.blocks[L.cond];
                cb.insts.clear();
                Inst jb;
                jb.op = Op::BR;
                jb.src1 = Operand::label(n ? copyLabel(fn.blocks[L.body].label, 0) : incLabel);
                cb.insts.push_back(jb);

                trace("unroll", fn.name, "loop at " + fn.blocks[L.cond].label + " unrolled " +
                      std::to_string(n) + " times");
                fn.blocks.erase(fn.blocks.begin() + L.body, fn.blocks.begin() + L.inc + 1);
                fn.blocks.insert(fn.blocks.begin() + L.body, out.begin(), out.end());
                changed = true;
            }
        }
    }
}


// ============================================================================
// Dead temporaries
// ============================================================================

// Removes pure instructions whose temporary result is never used (left by
// the other passes: a loop unroll's per-copy constant that every use folded
// away, a copy whose users were redirected). Loads (maybe volatile),
// variables' slots and memory-pinned vregs are never removed.
// *p++ / *p-- on a pointer variable S builds
//     %t = deref S;  %u = add %t, c;  store %u, S;  ... accesses through %t
// so the access goes through a copy of the old pointer. When nothing between
// the store and the last use of %t reads or writes S (or calls, or is asm),
// the increment moves after that use and %t becomes S + 0, which IRCodeGen
// folds into the accesses ((S),y with S in ZP):
//     %t = add S, 0;  ... accesses through %t;  %u = add S, c;  store %u, S
void optimizePostIncrementSink(Module& mod) {
    auto isTerm = [](Op op) { return op == Op::BR || op == Op::BR_COND || op == Op::SWITCH || op == Op::RET; };
    for (auto& fn : mod.functions) {
        if (!runOn(fn, "PostIncrementSink")) continue;
        std::map<uint32_t, int> defs, useCount;
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts) {
                if (in.dest.isVreg()) defs[in.dest.vregId]++;
                for (const Operand* o : {&in.src1, &in.src2})
                    if (o->isVreg()) useCount[o->vregId]++;
                for (const auto& a : in.args) if (a.isVreg()) useCount[a.vregId]++;
                for (const auto& ph : in.phiIncoming) if (ph.first.isVreg()) useCount[ph.first.vregId]++;
            }
        auto reads = [](const Inst& in, uint32_t v) {
            auto is = [v](const Operand& o) { return o.isVreg() && o.vregId == v; };
            if (is(in.src1) || is(in.src2)) return true;
            for (const auto& a : in.args) if (is(a)) return true;
            for (const auto& ph : in.phiIncoming) if (is(ph.first)) return true;
            return false;
        };
        for (auto& blk : fn.blocks) {
            auto& insts = blk.insts;
            for (size_t i = 0; i + 3 < insts.size(); i++) {
                Inst& d = insts[i];
                if (d.op != Op::DEREF || d.resultType != Type::PTR || !d.dest.isVreg() || !d.src1.isVreg()) continue;
                uint32_t t = d.dest.vregId, S = d.src1.vregId;
                if (!fn.localSlotVregs.count(S) || fn.memoryVregs.count(S) || fn.localSlotVregs.count(t) ||
                    defs[t] != 1) continue;
                Inst& a = insts[i + 1];
                const Inst& st = insts[i + 2];
                if ((a.op != Op::ADD && a.op != Op::SUB) || !a.dest.isVreg() || !a.src1.isVreg() ||
                    a.src1.vregId != t || !a.src2.isImm()) continue;
                uint32_t u = a.dest.vregId;
                if (defs[u] != 1 || useCount[u] != 1 || fn.localSlotVregs.count(u)) continue;
                if (st.op != Op::STORE || !st.src1.isVreg() || st.src1.vregId != u ||
                    !st.src2.isVreg() || st.src2.vregId != S) continue;
                // every other use of %t is in this block after the store
                int found = 0;
                size_t last = 0;
                for (size_t j = i + 3; j < insts.size(); j++)
                    if (reads(insts[j], t)) {
                        for (const Operand* o : {&insts[j].src1, &insts[j].src2})
                            if (o->isVreg() && o->vregId == t) found++;
                        for (const auto& x : insts[j].args) if (x.isVreg() && x.vregId == t) found++;
                        for (const auto& ph : insts[j].phiIncoming) if (ph.first.isVreg() && ph.first.vregId == t) found++;
                        last = j;
                    }
                if (found == 0 || found != useCount[t] - 1 || isTerm(insts[last].op)) continue;
                bool ok = true;
                for (size_t j = i + 3; j <= last && ok; j++) {
                    const Inst& x = insts[j];
                    if (x.op == Op::CALL || x.op == Op::CALL_VOID || x.op == Op::CALL_INDIRECT ||
                        x.op == Op::ASM_INLINE || x.op == Op::PHI || isTerm(x.op)) ok = false;
                    if ((x.dest.isVreg() && x.dest.vregId == S) || reads(x, S)) ok = false;
                    if (x.dest.isVreg() && x.dest.vregId == u) ok = false;
                }
                if (!ok) continue;
                d.op = Op::ADD;
                d.src2 = Operand::imm(0, Type::I16);
                a.src1 = Operand::vreg(S, Type::PTR);
                Inst ma = a, ms = st;
                trace("postinc", fn.name, "access before the increment of the pointer", d.loc);
                insts.insert(insts.begin() + last + 1, {ma, ms});
                insts.erase(insts.begin() + i + 1, insts.begin() + i + 3);
                useCount[t]--;   // (the increment no longer reads %t)
                useCount[S]++;
            }
        }
    }
}

// A leaf function (no calls, no inline asm) keeps its pointer locals in the
// ZP register pool (IRCodeGen: like register variables). Its pointer
// parameters arrive in their own storage (static allocation: _f__param_p,
// which same-file callers fill before entering at _f__sac; or a stack frame):
// one used twice or more is stored at the start of the body into a new local,
// a register variable, that every other use refers to.
bool paramsStayInZp(const Function& fn) {
    if (getenv("CC45_NO_ZP_PARAMS")) return false;
    if (fn.conv != CallConv::ZP || fn.isVariadic || fn.isRegparm || fn.isNaked || fn.isInterrupt ||
        fn.isNested || fn.paramTypes.empty() || fn.blocks.empty())
        return false;
    for (auto t : fn.paramTypes) if (t == Type::F32) return false;
    for (const auto& [v, t] : fn.vregTypes) if (t == Type::F32) return false;
    for (const auto& blk : fn.blocks)
        for (const auto& in : blk.insts) {
            // a call or asm can overwrite the block; float operations call the runtime
            if (in.op == Op::CALL || in.op == Op::CALL_VOID || in.op == Op::CALL_INDIRECT ||
                in.op == Op::ASM_INLINE || in.resultType == Type::F32 ||
                in.src1.type == Type::F32 || in.src2.type == Type::F32)
                return false;
        }
    return true;
}

void leafPointerParamsToZp(Module& mod) {
    if (getenv("CC45_NO_ZP_POINTERS")) return;
    for (auto& fn : mod.functions) {
        if (fn.blocks.empty() || fn.isNaked) continue;
        bool leaf = true;
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts)
                if (in.op == Op::CALL || in.op == Op::CALL_VOID || in.op == Op::CALL_INDIRECT || in.op == Op::ASM_INLINE)
                    leaf = false;
        if (!leaf) continue;
        // the copy must run once: no branch (or label address) to the entry block
        const std::string& entryLabel = fn.blocks[0].label;
        bool reentered = false;
        for (const auto& blk : fn.blocks)
            for (const auto& in : blk.insts) {
                for (const Operand* o : {&in.src1, &in.src2, &in.dest})
                    if (o->kind == OperandKind::LABEL && o->name == entryLabel) reentered = true;
                for (const auto& a : in.args)
                    if (a.kind == OperandKind::LABEL && a.name == entryLabel) reentered = true;
                for (const auto& c : in.switchCases) if (c.second == entryLabel) reentered = true;
            }
        if (reentered) continue;
        // (parameters left in the ZP parameter block are in ZP already)
        if (paramsStayInZp(fn)) continue;
        for (uint32_t p = 0; p < fn.paramTypes.size(); p++) {
            if (fn.paramTypes[p] != Type::PTR || fn.memoryVregs.count(p) || fn.registerVregs.count(p)) continue;
            auto uses = [&](const Operand& o) { return o.isVreg() && o.vregId == p; };
            int n = 0;
            for (const auto& blk : fn.blocks)
                for (const auto& in : blk.insts) {
                    if (uses(in.src1)) n++;
                    if (uses(in.src2)) n++;
                    if (uses(in.dest)) n++;
                    for (const auto& a : in.args) if (uses(a)) n++;
                    for (const auto& ph : in.phiIncoming) if (uses(ph.first)) n++;
                }
            if (n < 2) continue;
            uint32_t v = fn.allocVreg();
            for (auto& blk : fn.blocks)
                for (auto& in : blk.insts) {
                    if (uses(in.src1)) in.src1.vregId = v;
                    if (uses(in.src2)) in.src2.vregId = v;
                    if (uses(in.dest)) in.dest.vregId = v;
                    for (auto& a : in.args) if (uses(a)) a.vregId = v;
                    for (auto& ph : in.phiIncoming) if (uses(ph.first)) ph.first.vregId = v;
                }
            Inst st;
            st.op = Op::STORE;
            st.resultType = Type::PTR;
            st.src1 = Operand::vreg(p, Type::PTR);
            st.src2 = Operand::vreg(v, Type::PTR);
            auto& entry = fn.blocks[0].insts;
            if (!entry.empty()) st.loc = entry[0].loc;
            entry.insert(entry.begin(), st);
            fn.localSlotVregs.insert(v);
            fn.registerVregs.insert(v);
            fn.vregTypes[v] = Type::PTR;
            std::string name = "__zp_" + (p < fn.paramNames.size() && !fn.paramNames[p].empty()
                                          ? fn.paramNames[p] : std::to_string(p));
            fn.localNames[name] = v;
            fn.localNamesOrder.push_back(name);
            trace("zp", fn.name, "pointer parameter " + name.substr(5) + " in a ZP register (leaf)", st.loc);
        }
    }
}

void removeDeadTemps(Module& mod) {
    auto pure = [](Op op) {
        switch (op) {
            case Op::CONST: case Op::FCONST: case Op::COPY:
            case Op::ADD: case Op::SUB: case Op::MUL: case Op::MUL_U:
            case Op::AND: case Op::OR: case Op::XOR:
            case Op::LSL: case Op::LSR: case Op::ASR: case Op::SHL: case Op::SHR:
            case Op::NEG: case Op::NOT:
            case Op::CMP_EQ: case Op::CMP_NE: case Op::CMP_LT: case Op::CMP_LE:
            case Op::CMP_GT: case Op::CMP_GE: case Op::CMP_LTU: case Op::CMP_LEU:
            case Op::CMP_GTU: case Op::CMP_GEU:
            case Op::SEXT: case Op::ZEXT: case Op::TRUNC:
            case Op::ADDR_GLOBAL: case Op::ADDR_LOCAL: case Op::ADDR_LABEL: case Op::ADDR_ELEM:
                return true;
            default:
                return false;
        }
    };
    for (auto& fn : mod.functions) {
        bool changed = true;
        while (changed) {
            changed = false;
            std::set<uint32_t> used;
            for (const auto& b : fn.blocks)
                for (const auto& i : b.insts) {
                    if (i.src1.isVreg()) used.insert(i.src1.vregId);
                    if (i.src2.isVreg()) used.insert(i.src2.vregId);
                    for (const auto& a : i.args) if (a.isVreg()) used.insert(a.vregId);
                    for (const auto& p : i.phiIncoming) if (p.first.isVreg()) used.insert(p.first.vregId);
                }
            for (auto& b : fn.blocks)
                for (auto& i : b.insts) {
                    if (!pure(i.op) || !i.dest.isVreg()) continue;
                    uint32_t d = i.dest.vregId;
                    if (used.count(d) || fn.localSlotVregs.count(d) || fn.memoryVregs.count(d)) continue;
                    if ((int)d == fn.staticLinkVreg || (int)d == fn.retAddrSlot) continue;
                    i = Inst();
                    i.op = Op::NOP;
                    changed = true;
                }
        }
    }
}

} // namespace ir
