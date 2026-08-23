#include "AlgebraicSimplification.hpp"
#include <algorithm>

AlgebraicSimplification::AlgebraicSimplification()
    : OptimizationPassBase(OptimizationType::ALGEBRAIC_SIMPLIFICATION,
                          "Algebraic Simplification") {
}

void AlgebraicSimplification::apply(ir::Module& irModule) {
    // Phase C6.2: Algebraic simplification at IR level
    // Walk all instructions and apply identity/annihilator elimination

    for (auto& func : irModule.functions) {
        for (auto& block : func.blocks) {
            for (size_t i = 0; i < block.insts.size(); ++i) {
                auto& inst = block.insts[i];

                // Skip non-arithmetic operations
                if (inst.op == ir::Op::CALL || inst.op == ir::Op::BR ||
                    inst.op == ir::Op::BR_COND || inst.op == ir::Op::RET) {
                    continue;
                }

                // Try to simplify instruction
                if (simplifyInstruction(inst)) {
                    metrics_.instructionsOptimized++;
                    metrics_.codeReductionBytes += 2;
                }
            }
        }
    }

    // Report metrics
    if (identitiesEliminated_ + annihilatorsEliminated_ +
        shortCircuitsApplied_ + shiftsSimplified_ > 0) {
        
        metrics_.instructionsOptimized = identitiesEliminated_ +
                                        annihilatorsEliminated_ +
                                        shortCircuitsApplied_ +
                                        shiftsSimplified_;
    }
}

bool AlgebraicSimplification::simplifyInstruction(ir::Inst& inst) {
    // Check if src2 is a constant
    if (inst.src2.kind != ir::OperandKind::IMM) {
        return false;
    }

    unsigned val = inst.src2.immVal;

    // Check for arithmetic identities (a OP 1 = a, a OP 0 = a, etc.)
    if (isArithmeticIdentity(inst.op, val)) {
        // Replace with src1 (copy operation)
        inst.op = ir::Op::COPY;
        inst.src2 = ir::Operand();  // Clear src2
        identitiesEliminated_++;
        return true;
    }

    // Check for arithmetic annihilators (a * 0 = 0, a & 0 = 0, etc.)
    if (isArithmeticAnnihilator(inst.op, val)) {
        // Replace with constant 0
        inst.op = ir::Op::CONST;
        inst.src1 = ir::Operand::imm(0, ir::Type::I8);
        inst.src2 = ir::Operand();
        annihilatorsEliminated_++;
        return true;
    }

    // Check for bitwise identities
    if (isBitwiseIdentity(inst.op, val)) {
        // Replace with src1 (copy operation)
        inst.op = ir::Op::COPY;
        inst.src2 = ir::Operand();
        identitiesEliminated_++;
        return true;
    }

    // Check for bitwise annihilators
    if (isBitwiseAnnihilator(inst.op, val)) {
        // Replace with constant 0
        inst.op = ir::Op::CONST;
        inst.src1 = ir::Operand::imm(0, ir::Type::I8);
        inst.src2 = ir::Operand();
        annihilatorsEliminated_++;
        return true;
    }

    // Check for identity shifts (a << 0 = a, a >> 0 = a)
    if (isIdentityShift(inst.op, val)) {
        // Replace with src1 (copy operation)
        inst.op = ir::Op::COPY;
        inst.src2 = ir::Operand();
        shiftsSimplified_++;
        return true;
    }

    // Check for boolean short-circuits
    ir::Operand result;
    if (isBooleanShortCircuit(inst.op, val, result)) {
        // Replace with constant result
        inst.op = ir::Op::CONST;
        inst.src1 = result;
        inst.src2 = ir::Operand();
        shortCircuitsApplied_++;
        return true;
    }

    return false;
}

bool AlgebraicSimplification::isArithmeticIdentity(ir::Op op, unsigned val) const {
    switch (op) {
        case ir::Op::MUL: return val == 1;   // a * 1 = a
        case ir::Op::DIV: return val == 1;   // a / 1 = a
        case ir::Op::ADD: return val == 0;   // a + 0 = a
        case ir::Op::SUB: return val == 0;   // a - 0 = a
        default: return false;
    }
}

bool AlgebraicSimplification::isArithmeticAnnihilator(ir::Op op, unsigned val) const {
    switch (op) {
        case ir::Op::MUL: return val == 0;   // a * 0 = 0
        case ir::Op::AND: return val == 0;   // a & 0 = 0
        default: return false;
    }
}

bool AlgebraicSimplification::isBitwiseIdentity(ir::Op op, unsigned val) const {
    switch (op) {
        case ir::Op::OR:  return val == 0;      // a | 0 = a
        case ir::Op::XOR: return val == 0;      // a ^ 0 = a
        case ir::Op::AND: return val == ~0U;    // a & ~0 = a
        default: return false;
    }
}

bool AlgebraicSimplification::isBitwiseAnnihilator(ir::Op op, unsigned val) const {
    switch (op) {
        case ir::Op::AND: return val == 0;      // a & 0 = 0
        case ir::Op::OR:  return val == ~0U;    // a | ~0 = ~0
        default: return false;
    }
}

bool AlgebraicSimplification::isIdentityShift(ir::Op op, unsigned shiftAmount) const {
    return shiftAmount == 0 && (op == ir::Op::SHL || op == ir::Op::SHR);
}

bool AlgebraicSimplification::isBooleanShortCircuit(ir::Op op, unsigned val,
                                                    ir::Operand& result) const {
    switch (op) {
        case ir::Op::OR:
            if (val != 0) {  // a || 1 = 1
                result = ir::Operand::imm(0, ir::Type::I8);
                return true;
            }
            break;

        case ir::Op::AND:
            if (val == 0) {  // a && 0 = 0
                result = ir::Operand::imm(0, ir::Type::I8);
                return true;
            }
            break;

        default:
            break;
    }
    return false;
}
