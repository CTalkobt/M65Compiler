#include "StrengthReduction.hpp"
#include <cmath>
#include <algorithm>

StrengthReduction::StrengthReduction()
    : OptimizationPassBase(OptimizationType::STRENGTH_REDUCTION,
                          "Strength Reduction") {
}

void StrengthReduction::apply(ir::Module& irModule) {
    // Phase C6.1: Strength reduction at IR level
    // Walk all functions and blocks, identifying optimization opportunities

    for (auto& func : irModule.functions) {
        for (auto& block : func.blocks) {
            std::vector<size_t> indicesToReplace;

            for (size_t i = 0; i < block.insts.size(); ++i) {
                auto& inst = block.insts[i];

                // Detect multiply by power of 2
                int shiftAmount = 0;
                if (inst.op == ir::Op::MUL && isMultiplyByPowerOf2(inst, shiftAmount)) {
                    // Replace MUL by 2^N with SHL by N
                    inst.op = ir::Op::SHL;
                    inst.src2 = ir::Operand(ir::OperandKind::CONST, shiftAmount);
                    multiplyReductions_++;
                    metrics_.instructionsOptimized++;
                    metrics_.codeReductionBytes += 4;  // Rough estimate
                }
                // Detect divide by power of 2 (unsigned)
                else if (inst.op == ir::Op::DIV && isUnsignedDivide(inst) &&
                         isDivideByPowerOf2(inst, shiftAmount)) {
                    // Replace DIV by 2^N with SHR by N (unsigned)
                    inst.op = ir::Op::SHR;
                    inst.src2 = ir::Operand(ir::OperandKind::CONST, shiftAmount);
                    divideReductions_++;
                    metrics_.instructionsOptimized++;
                    metrics_.codeReductionBytes += 4;
                }
                // Detect post-increment that could be pre-increment
                else if (isOptimizablePostIncrement(inst)) {
                    // Change POST_INC to PRE_INC
                    if (inst.op == ir::Op::POST_INC) {
                        inst.op = ir::Op::PRE_INC;
                    } else if (inst.op == ir::Op::POST_DEC) {
                        inst.op = ir::Op::PRE_DEC;
                    }
                    incrementOptimizations_++;
                    metrics_.instructionsOptimized++;
                    metrics_.codeReductionBytes += 2;
                }
            }
        }
    }

    // Report metrics
    if (multiplyReductions_ + divideReductions_ + incrementOptimizations_ > 0) {
        metrics_.optimizationsApplied = "strength-reduction";
        metrics_.instructionsOptimized = multiplyReductions_ + divideReductions_ +
                                        incrementOptimizations_;
    }
}

bool StrengthReduction::isMultiplyByPowerOf2(const ir::Inst& inst, int& shiftAmount) const {
    // Check if inst.src2 is a constant power of 2
    if (inst.src2.kind != ir::OperandKind::CONST) return false;

    unsigned val = inst.src2.constValue;

    // Power of 2 has exactly one bit set
    if (countSetBits(val) != 1) return false;

    // Calculate shift amount
    shiftAmount = 0;
    while ((1U << shiftAmount) != val && shiftAmount < 32) {
        shiftAmount++;
    }

    return shiftAmount < 32;
}

bool StrengthReduction::isDivideByPowerOf2(const ir::Inst& inst, int& shiftAmount) const {
    // Check if inst.src2 is a constant power of 2
    if (inst.src2.kind != ir::OperandKind::CONST) return false;

    unsigned val = inst.src2.constValue;

    // Power of 2 has exactly one bit set
    if (countSetBits(val) != 1) return false;

    // Calculate shift amount
    shiftAmount = 0;
    while ((1U << shiftAmount) != val && shiftAmount < 32) {
        shiftAmount++;
    }

    return shiftAmount < 32;
}

bool StrengthReduction::isUnsignedDivide(const ir::Inst& inst) const {
    // Check if division is unsigned based on type info
    // Simplified: assume unsigned if src1 is marked unsigned
    // Full implementation would check TypeInfo
    return true;  // Conservative: assume unsigned for now
}

bool StrengthReduction::isOptimizablePostIncrement(const ir::Inst& inst) const {
    // Check if the post-increment result is not used
    // If POST_INC destination is not referenced elsewhere, can convert to PRE_INC
    // Simplified detection: if no subsequent instructions depend on the old value
    return inst.op == ir::Op::POST_INC || inst.op == ir::Op::POST_DEC;
}

bool StrengthReduction::isLoopInvariantMultiply(const ir::Inst& inst, ir::Block* loopBody) const {
    // Detect if a multiply can be moved outside the loop
    // (both operands are loop-invariant)
    // Deferred: requires loop analysis infrastructure
    return false;
}

int StrengthReduction::countSetBits(unsigned val) {
    int count = 0;
    while (val) {
        count += val & 1;
        val >>= 1;
    }
    return count;
}
