#include "GlobalValueNumbering.hpp"
#include <sstream>

GlobalValueNumbering::GlobalValueNumbering()
    : OptimizationPassBase(OptimizationType::GLOBAL_VALUE_NUMBERING,
                          "Global Value Numbering") {
}

void GlobalValueNumbering::apply(ir::Module& irModule) {
    // Phase C7.5: Global Value Numbering at IR level
    // Track value identities across blocks

    for (auto& func : irModule.functions) {
        BlockGVNState state;

        for (auto& block : func.blocks) {
            for (size_t i = 0; i < block.insts.size(); ++i) {
                auto& inst = block.insts[i];

                // Skip memory operations and calls (invalidate state)
                if (inst.op == ir::Op::STORE || inst.op == ir::Op::CALL) {
                    state.exprToValue.clear();
                    continue;
                }

                // For eligible operations, compute value number
                if (inst.op == ir::Op::ADD || inst.op == ir::Op::SUB ||
                    inst.op == ir::Op::MUL || inst.op == ir::Op::DIV ||
                    inst.op == ir::Op::AND || inst.op == ir::Op::OR ||
                    inst.op == ir::Op::XOR || inst.op == ir::Op::SHL ||
                    inst.op == ir::Op::SHR || inst.op == ir::Op::CMP_EQ) {

                    size_t valNum = getValueNumber(inst, state);

                    // Check if we've already computed this value
                    auto it = state.valueNumbers.find(valNum);
                    if (it != state.valueNumbers.end() && it->second.firstInstIdx < i) {
                        // Reuse previous computation
                        inst.op = ir::Op::COPY;
                        inst.src1 = ir::Operand::vreg(
                                              std::stoul(it->second.resultVreg.substr(6)));
                        inst.src2 = ir::Operand();
                        valuesReused_++;
                        metrics_.instructionsOptimized++;
                        metrics_.codeReductionBytes += 4;
                    } else {
                        // Record this value number
                        ValueNumber vn;
                        vn.number = valNum;
                        vn.expr = buildExpressionKey(inst.op, inst.src1, inst.src2);
                        vn.firstInstIdx = i;
                        vn.resultVreg = "vreg_" + std::to_string(inst.dest.vregId);
                        state.valueNumbers[valNum] = vn;
                        equivalencesFound_++;
                    }
                }
            }
        }
    }

    // Report metrics
    if (valuesReused_ > 0 || equivalencesFound_ > 0) {
        
        metrics_.instructionsOptimized = valuesReused_;
    }
}

size_t GlobalValueNumbering::getValueNumber(const ir::Inst& inst,
                                           BlockGVNState& state) {
    // Build unique expression key
    std::string key = buildExpressionKey(inst.op, inst.src1, inst.src2);

    // Check if we've seen this expression before
    auto it = state.exprToValue.find(key);
    if (it != state.exprToValue.end()) {
        return it->second;  // Reuse existing value number
    }

    // Assign new value number
    size_t newValueNum = state.nextValueNumber++;
    state.exprToValue[key] = newValueNum;
    return newValueNum;
}

std::string GlobalValueNumbering::buildExpressionKey(ir::Op op,
                                                     const ir::Operand& src1,
                                                     const ir::Operand& src2) const {
    std::ostringstream oss;
    oss << static_cast<int>(op) << ":";

    // Normalize src1
    switch (src1.kind) {
        case ir::OperandKind::VREG:
            oss << "v" << src1.vregId << ":";
            break;
        case ir::OperandKind::IMM:
            oss << "c" << src1.immVal << ":";
            break;
        case ir::OperandKind::GLOBAL:
            oss << "s:" << src1.name << ":";
            break;
        default:
            oss << "?:";
    }

    // Normalize src2
    switch (src2.kind) {
        case ir::OperandKind::VREG:
            oss << "v" << src2.vregId;
            break;
        case ir::OperandKind::IMM:
            oss << "c" << src2.immVal;
            break;
        case ir::OperandKind::GLOBAL:
            oss << "s:" << src2.name;
            break;
        default:
            oss << "?";
    }

    return oss.str();
}

bool GlobalValueNumbering::hasValueNumber(const std::string& key,
                                         const BlockGVNState& state) const {
    return state.exprToValue.count(key) > 0;
}
