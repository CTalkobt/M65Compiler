#include "CommonSubexpressionElimination.hpp"
#include <sstream>

CommonSubexpressionElimination::CommonSubexpressionElimination()
    : OptimizationPassBase(OptimizationType::CSE,
                          "Common Subexpression Elimination") {
}

void CommonSubexpressionElimination::apply(ir::Module& irModule) {
    // Phase C7.2: CSE at IR level
    // Walk blocks and eliminate redundant expressions

    for (auto& func : irModule.functions) {
        for (auto& block : func.blocks) {
            BlockCSEState state;
            std::vector<size_t> indicesToRemove;

            for (size_t i = 0; i < block.insts.size(); ++i) {
                auto& inst = block.insts[i];

                // Check if this is a computable expression
                if (isCSEEligible(inst.op)) {
                    std::string src1 = normalizeOperand(inst.src1);
                    std::string src2 = normalizeOperand(inst.src2);
                    std::string exprHash = hashExpression(inst.op, src1, src2);

                    // Check if we've seen this expression before
                    auto it = state.expressions.find(exprHash);
                    if (it != state.expressions.end() && it->second.isValid) {
                        // Redundant expression! Replace with copy
                        inst.op = ir::Op::COPY;
                        // Parse vreg_N to get ID
                        size_t vregId = std::stoul(it->second.vreg.substr(6));
                        inst.src1 = ir::Operand::vreg(vregId, ir::Type::I32);
                        inst.src2 = ir::Operand::none();
                        redundantExpressions_++;
                        metrics_.instructionsOptimized++;
                        metrics_.codeReductionBytes += 4;
                    } else {
                        // First occurrence, record it
                        ExpressionValue ev;
                        ev.op = inst.op;
                        ev.src1 = src1;
                        ev.src2 = src2;
                        ev.vreg = "vreg_" + std::to_string(inst.dest.vregId);
                        ev.instIndex = i;
                        ev.isValid = true;
                        state.expressions[exprHash] = ev;
                    }
                }
                // Memory operations invalidate expressions
                else if (inst.op == ir::Op::STORE || inst.op == ir::Op::CALL) {
                    // Invalidate all expressions (conservative)
                    for (auto& [hash, expr] : state.expressions) {
                        expr.isValid = false;
                    }
                    state.clobberedVregs.clear();
                }
            }
        }
    }

    // Report metrics
    if (redundantExpressions_ > 0) {
        metrics_.instructionsOptimized = redundantExpressions_;
    }
}

std::string CommonSubexpressionElimination::normalizeOperand(const ir::Operand& op) const {
    switch (op.kind) {
        case ir::OperandKind::VREG:
            return "v" + std::to_string(op.vregId);
        case ir::OperandKind::IMM:
            return "c" + std::to_string(op.immVal);
        case ir::OperandKind::GLOBAL:
            return "s:" + op.name;
        default:
            return "?";
    }
}

std::string CommonSubexpressionElimination::hashExpression(ir::Op op,
                                                          const std::string& src1,
                                                          const std::string& src2) const {
    std::ostringstream oss;
    oss << static_cast<int>(op) << ":" << src1 << ":" << src2;
    return oss.str();
}

bool CommonSubexpressionElimination::isCSEEligible(ir::Op op) const {
    // Expressions with no side effects can be CSE'd
    switch (op) {
        case ir::Op::ADD:
        case ir::Op::SUB:
        case ir::Op::MUL:
        case ir::Op::DIV:
        case ir::Op::AND:
        case ir::Op::OR:
        case ir::Op::XOR:
        case ir::Op::SHL:
        case ir::Op::SHR:
        case ir::Op::CMP_EQ:
        case ir::Op::CMP_NE:
        case ir::Op::CMP_LT:
        case ir::Op::CMP_LE:
        case ir::Op::CMP_GT:
        case ir::Op::CMP_GE:
            return true;
        default:
            return false;
    }
}

bool CommonSubexpressionElimination::clobbersExpression(const ir::Inst& inst,
                                                       const ExpressionValue& expr) const {
    // Check if instruction invalidates the expression
    if (inst.op == ir::Op::STORE || inst.op == ir::Op::CALL) {
        return true;  // Conservative: any memory op invalidates
    }
    return false;
}
