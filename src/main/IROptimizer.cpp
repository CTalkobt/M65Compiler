#include "IROptimizer.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace ir {

void IROptimizer::optimize(Module& mod) {
    for (auto& fn : mod.functions) {
        bool changed = true;
        int iterations = 0;
        const int maxIterations = 10;

        while (changed && iterations < maxIterations) {
            changed = false;
            iterations++;

            if (foldConstants(fn)) changed = true;
            if (propagateConstants(fn)) changed = true;
            if (reduceStrength(fn)) changed = true;
            if (simplifyControlFlow(fn)) changed = true;
            if (eliminateUnreachableBlocks(fn)) changed = true;
            if (propagateCopies(fn)) changed = true;
            if (eliminateDeadCode(fn)) changed = true;
        }

        if (verboseLevel_ >= 1 && iterations > 1) {
            std::cout << "IR Optimization for function @" << fn.name << " converged in " << iterations << " iterations." << std::endl;
        }
    }
}

Operand IROptimizer::evaluateBinaryOp(Op op, Type type, int64_t v1, int64_t v2) {
    int64_t res = 0;
    bool valid = true;

    switch (op) {
        case Op::ADD: res = v1 + v2; break;
        case Op::SUB: res = v1 - v2; break;
        case Op::MUL: res = v1 * v2; break;
        case Op::DIV:
            if (v2 != 0) res = v1 / v2;
            else valid = false;
            break;
        case Op::MOD:
            if (v2 != 0) res = v1 % v2;
            else valid = false;
            break;
        case Op::MUL_U: res = (uint64_t)v1 * (uint64_t)v2; break;
        case Op::DIV_U:
            if (v2 != 0) res = (uint64_t)v1 / (uint64_t)v2;
            else valid = false;
            break;
        case Op::MOD_U:
            if (v2 != 0) res = (uint64_t)v1 % (uint64_t)v2;
            else valid = false;
            break;
        case Op::AND: res = v1 & v2; break;
        case Op::OR:  res = v1 | v2; break;
        case Op::XOR: res = v1 ^ v2; break;
        case Op::LSL:
        case Op::SHL: res = v1 << v2; break;
        case Op::LSR:
        case Op::SHR: res = (uint64_t)v1 >> v2; break;
        case Op::ASR: res = v1 >> v2; break;

        case Op::CMP_EQ: res = (v1 == v2) ? 1 : 0; break;
        case Op::CMP_NE: res = (v1 != v2) ? 1 : 0; break;
        case Op::CMP_LT: res = (v1 < v2) ? 1 : 0; break;
        case Op::CMP_LE: res = (v1 <= v2) ? 1 : 0; break;
        case Op::CMP_GT: res = (v1 > v2) ? 1 : 0; break;
        case Op::CMP_GE: res = (v1 >= v2) ? 1 : 0; break;

        case Op::CMP_LTU: res = ((uint64_t)v1 < (uint64_t)v2) ? 1 : 0; break;
        case Op::CMP_LEU: res = ((uint64_t)v1 <= (uint64_t)v2) ? 1 : 0; break;
        case Op::CMP_GTU: res = ((uint64_t)v1 > (uint64_t)v2) ? 1 : 0; break;
        case Op::CMP_GEU: res = ((uint64_t)v1 >= (uint64_t)v2) ? 1 : 0; break;

        default: valid = false; break;
    }

    if (!valid) return Operand::none();

    // Mask to type size
    int size = typeSize(type);
    if (size == 1) res = (int8_t)res;
    else if (size == 2) res = (int16_t)res;
    else if (size == 4) res = (int32_t)res;

    return Operand::imm(res, type);
}

Operand IROptimizer::evaluateUnaryOp(Op op, Type type, int64_t val) {
    int64_t res = 0;
    bool valid = true;

    switch (op) {
        case Op::NEG: res = -val; break;
        case Op::NOT: res = ~val; break;
        case Op::SEXT: res = val; break; // Sign extend is implicit in int64_t if properly masked
        case Op::ZEXT:
            if (type == Type::I8) res = (uint8_t)val;
            else if (type == Type::I16) res = (uint16_t)val;
            else res = (uint32_t)val;
            break;
        case Op::TRUNC: res = val; break;
        default: valid = false; break;
    }

    if (!valid) return Operand::none();

    int size = typeSize(type);
    if (size == 1) res = (int8_t)res;
    else if (size == 2) res = (int16_t)res;
    else if (size == 4) res = (int32_t)res;

    return Operand::imm(res, type);
}

bool IROptimizer::foldConstants(Function& fn) {
    bool changed = false;
    std::map<uint32_t, Operand> constMap;

    for (auto& block : fn.blocks) {
        for (auto& inst : block.insts) {
            // 1. Substitute operands from constMap
            if (inst.src1.isVreg() && constMap.count(inst.src1.vregId)) {
                inst.src1 = constMap[inst.src1.vregId];
                changed = true;
            }
            if (inst.src2.isVreg() && constMap.count(inst.src2.vregId)) {
                inst.src2 = constMap[inst.src2.vregId];
                changed = true;
            }

            // 2. Evaluate instruction if operands are immediates
            if (inst.op == Op::CONST) {
                if (inst.dest.isVreg()) {
                    constMap[inst.dest.vregId] = inst.src1;
                }
            } else if (inst.src1.isImm() && inst.src2.isImm()) {
                Operand folded = evaluateBinaryOp(inst.op, inst.resultType, inst.src1.immVal, inst.src2.immVal);
                if (!folded.isNone() && inst.dest.isVreg()) {
                    inst.op = Op::CONST;
                    inst.src1 = folded;
                    inst.src2 = Operand::none();
                    constMap[inst.dest.vregId] = folded;
                    changed = true;
                }
            } else if (inst.src1.isImm() && inst.src2.isNone()) {
                Operand folded = evaluateUnaryOp(inst.op, inst.resultType, inst.src1.immVal);
                if (!folded.isNone() && inst.dest.isVreg()) {
                    inst.op = Op::CONST;
                    inst.src1 = folded;
                    constMap[inst.dest.vregId] = folded;
                    changed = true;
                }
            }
        }
    }

    return changed;
}

bool IROptimizer::reduceStrength(Function& fn) {
    bool changed = false;

    for (auto& block : fn.blocks) {
        for (auto& inst : block.insts) {
            // Strength reduction: MUL by power of 2 -> LSL / SHL
            if (inst.op == Op::MUL || inst.op == Op::MUL_U) {
                if (inst.src2.isImm() && inst.src2.immVal > 0) {
                    int64_t val = inst.src2.immVal;
                    if ((val & (val - 1)) == 0) { // Check power of 2
                        int shift = 0;
                        while (val > 1) { val >>= 1; shift++; }
                        inst.op = Op::LSL;
                        inst.src2 = Operand::imm(shift, Type::I8);
                        changed = true;
                    }
                } else if (inst.src1.isImm() && inst.src1.immVal > 0) {
                    int64_t val = inst.src1.immVal;
                    if ((val & (val - 1)) == 0) {
                        int shift = 0;
                        while (val > 1) { val >>= 1; shift++; }
                        inst.op = Op::LSL;
                        inst.src1 = inst.src2;
                        inst.src2 = Operand::imm(shift, Type::I8);
                        changed = true;
                    }
                }
            }

            // Identity operations: ADD x, 0 -> COPY x; MUL x, 1 -> COPY x
            if (inst.op == Op::ADD && inst.src2.isImm() && inst.src2.immVal == 0) {
                inst.op = Op::COPY;
                inst.src2 = Operand::none();
                changed = true;
            } else if (inst.op == Op::MUL && inst.src2.isImm() && inst.src2.immVal == 1) {
                inst.op = Op::COPY;
                inst.src2 = Operand::none();
                changed = true;
            }
        }
    }

    return changed;
}

bool IROptimizer::simplifyControlFlow(Function& fn) {
    bool changed = false;

    for (auto& block : fn.blocks) {
        for (auto& inst : block.insts) {
            // Fold BR_COND with constant condition
            // BR_COND layout: src1=condition, dest=trueLabel, src2=falseLabel
            if (inst.op == Op::BR_COND && inst.src1.isImm()) {
                std::string targetLabel = (inst.src1.immVal != 0) ? inst.dest.name : inst.src2.name;
                inst.op = Op::BR;
                inst.dest = Operand::none();
                inst.src1 = Operand::label(targetLabel);
                inst.src2 = Operand::none();
                inst.args.clear();
                changed = true;
            }
        }
    }

    return changed;
}

bool IROptimizer::eliminateDeadCode(Function& fn) {
    bool changed = false;
    std::set<uint32_t> usedVregs;

    // Collect all used vregs
    for (const auto& block : fn.blocks) {
        for (const auto& inst : block.insts) {
            if (inst.src1.isVreg()) usedVregs.insert(inst.src1.vregId);
            if (inst.src2.isVreg()) usedVregs.insert(inst.src2.vregId);
            for (const auto& arg : inst.args) {
                if (arg.isVreg()) usedVregs.insert(arg.vregId);
            }
            for (const auto& phi : inst.phiIncoming) {
                if (phi.first.isVreg()) usedVregs.insert(phi.first.vregId);
            }
        }
    }

    // Remove instructions whose destination vreg is never used (and has no side-effects)
    for (auto& block : fn.blocks) {
        auto it = block.insts.begin();
        while (it != block.insts.end()) {
            bool isSideEffectFree = false;
            switch (it->op) {
                case Op::CONST:
                case Op::ADD: case Op::SUB: case Op::MUL: case Op::DIV: case Op::MOD:
                case Op::MUL_U: case Op::DIV_U: case Op::MOD_U:
                case Op::AND: case Op::OR: case Op::XOR: case Op::LSL: case Op::LSR: case Op::ASR:
                case Op::SHL: case Op::SHR: case Op::NEG: case Op::NOT:
                case Op::CMP_EQ: case Op::CMP_NE: case Op::CMP_LT: case Op::CMP_LE: case Op::CMP_GT: case Op::CMP_GE:
                case Op::CMP_LTU: case Op::CMP_LEU: case Op::CMP_GTU: case Op::CMP_GEU:
                case Op::SEXT: case Op::ZEXT: case Op::TRUNC: case Op::COPY:
                case Op::ADDR_GLOBAL: case Op::ADDR_LOCAL: case Op::ADDR_LABEL: case Op::ADDR_ELEM:
                    isSideEffectFree = true;
                    break;
                default:
                    isSideEffectFree = false;
                    break;
            }

            if (isSideEffectFree && it->dest.isVreg() && usedVregs.count(it->dest.vregId) == 0) {
                it = block.insts.erase(it);
                changed = true;
            } else {
                ++it;
            }
        }
    }

    return changed;
}

bool IROptimizer::eliminateUnreachableBlocks(Function& fn) {
    if (fn.blocks.empty()) return false;

    bool changed = false;
    std::set<size_t> reachable;
    std::vector<size_t> worklist;

    // Entry block is always reachable
    reachable.insert(0);
    worklist.push_back(0);

    // Worklist traversal to mark all reachable blocks
    while (!worklist.empty()) {
        size_t idx = worklist.back();
        worklist.pop_back();

        if (idx >= fn.blocks.size()) continue;

        const auto& block = fn.blocks[idx];

        // Find successors from terminator
        if (!block.insts.empty()) {
            const auto& term = block.insts.back();

            if (term.op == Op::BR) {
                // Unconditional branch
                if (term.src1.kind == OperandKind::LABEL) {
                    for (size_t i = 0; i < fn.blocks.size(); i++) {
                        if (fn.blocks[i].label == term.src1.name && reachable.insert(i).second) {
                            worklist.push_back(i);
                        }
                    }
                }
            } else if (term.op == Op::BR_COND) {
                // Conditional branch - both paths reachable
                if (term.dest.kind == OperandKind::LABEL) {
                    for (size_t i = 0; i < fn.blocks.size(); i++) {
                        if (fn.blocks[i].label == term.dest.name && reachable.insert(i).second) {
                            worklist.push_back(i);
                        }
                    }
                }
                if (term.src2.kind == OperandKind::LABEL) {
                    for (size_t i = 0; i < fn.blocks.size(); i++) {
                        if (fn.blocks[i].label == term.src2.name && reachable.insert(i).second) {
                            worklist.push_back(i);
                        }
                    }
                }
            } else {
                // Other terminators or fall-through: next block is reachable
                if (idx + 1 < fn.blocks.size() && reachable.insert(idx + 1).second) {
                    worklist.push_back(idx + 1);
                }
            }
        } else {
            // Empty block: fall through to next
            if (idx + 1 < fn.blocks.size() && reachable.insert(idx + 1).second) {
                worklist.push_back(idx + 1);
            }
        }
    }

    // Remove unreachable blocks
    auto it = fn.blocks.begin();
    size_t idx = 0;
    while (it != fn.blocks.end()) {
        if (!reachable.count(idx)) {
            it = fn.blocks.erase(it);
            changed = true;
        } else {
            ++it;
            ++idx;
        }
    }

    return changed;
}

bool IROptimizer::propagateConstants(Function& fn) {
    bool changed = false;
    std::map<uint32_t, Operand> globalConstMap;

    // Iterate blocks to propagate constants across block boundaries
    for (auto& block : fn.blocks) {
        std::map<uint32_t, Operand> blockConstMap = globalConstMap;

        for (auto& inst : block.insts) {
            // Substitute operands from blockConstMap
            if (inst.src1.isVreg() && blockConstMap.count(inst.src1.vregId)) {
                inst.src1 = blockConstMap[inst.src1.vregId];
                changed = true;
            }
            if (inst.src2.isVreg() && blockConstMap.count(inst.src2.vregId)) {
                inst.src2 = blockConstMap[inst.src2.vregId];
                changed = true;
            }

            // For phi nodes: check if all incoming values are the same constant
            if (inst.op == Op::PHI && inst.dest.isVreg()) {
                bool allConstant = !inst.phiIncoming.empty();
                Operand constVal = Operand::none();

                for (const auto& [val, _] : inst.phiIncoming) {
                    if (blockConstMap.count(val.vregId ? val.vregId : 0xFFFFFFFF)) {
                        Operand mapped = blockConstMap[val.vregId];
                        if (constVal.isNone()) {
                            constVal = mapped;
                        } else if (constVal.immVal != mapped.immVal) {
                            allConstant = false;
                            break;
                        }
                    } else if (!val.isImm()) {
                        allConstant = false;
                        break;
                    } else if (constVal.isNone()) {
                        constVal = val;
                    } else if (constVal.immVal != val.immVal) {
                        allConstant = false;
                        break;
                    }
                }

                if (allConstant && !constVal.isNone()) {
                    inst.op = Op::CONST;
                    inst.src1 = constVal;
                    inst.phiIncoming.clear();
                    blockConstMap[inst.dest.vregId] = constVal;
                    changed = true;
                }
            } else if (inst.op == Op::CONST && inst.dest.isVreg()) {
                blockConstMap[inst.dest.vregId] = inst.src1;
            }
        }

        globalConstMap = blockConstMap;
    }

    return changed;
}

bool IROptimizer::propagateCopies(Function& fn) {
    bool changed = false;

    // Build a map of copy assignments: vreg -> source operand
    std::map<uint32_t, Operand> copyMap;
    std::map<uint32_t, int> useCount;

    // First pass: collect all uses and copy assignments
    for (const auto& block : fn.blocks) {
        for (const auto& inst : block.insts) {
            if (inst.op == Op::COPY && inst.dest.isVreg()) {
                copyMap[inst.dest.vregId] = inst.src1;
            }

            if (inst.src1.isVreg()) useCount[inst.src1.vregId]++;
            if (inst.src2.isVreg()) useCount[inst.src2.vregId]++;
            for (const auto& arg : inst.args) {
                if (arg.isVreg()) useCount[arg.vregId]++;
            }
            for (const auto& [val, _] : inst.phiIncoming) {
                if (val.isVreg()) useCount[val.vregId]++;
            }
        }
    }

    // Second pass: replace uses of copy destinations with sources
    for (auto& block : fn.blocks) {
        for (auto& inst : block.insts) {
            // Replace src1
            if (inst.src1.isVreg() && copyMap.count(inst.src1.vregId)) {
                Operand src = copyMap[inst.src1.vregId];
                if (src.isVreg() || src.isImm()) {
                    inst.src1 = src;
                    changed = true;
                }
            }

            // Replace src2
            if (inst.src2.isVreg() && copyMap.count(inst.src2.vregId)) {
                Operand src = copyMap[inst.src2.vregId];
                if (src.isVreg() || src.isImm()) {
                    inst.src2 = src;
                    changed = true;
                }
            }

            // Replace args
            for (auto& arg : inst.args) {
                if (arg.isVreg() && copyMap.count(arg.vregId)) {
                    Operand src = copyMap[arg.vregId];
                    if (src.isVreg() || src.isImm()) {
                        arg = src;
                        changed = true;
                    }
                }
            }
        }
    }

    // Third pass: remove COPY instructions whose destination is never used
    // (after propagation, the copy becomes dead)
    for (auto& block : fn.blocks) {
        auto it = block.insts.begin();
        while (it != block.insts.end()) {
            if (it->op == Op::COPY && it->dest.isVreg() && useCount[it->dest.vregId] == 0) {
                it = block.insts.erase(it);
                changed = true;
            } else {
                ++it;
            }
        }
    }

    return changed;
}

} // namespace ir
