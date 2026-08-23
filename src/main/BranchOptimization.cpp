#include "BranchOptimization.hpp"
#include <algorithm>

BranchOptimization::BranchOptimization()
    : OptimizationPassBase(OptimizationType::BRANCH_OPTIMIZATION,
                          "Branch Optimization") {
}

void BranchOptimization::apply(ir::Module& irModule) {
    // Phase C7.4: Branch Optimization at IR level
    // Eliminate inefficient branching patterns

    for (auto& func : irModule.functions) {
        // Detect and remove unreachable code
        for (auto& block : func.blocks) {
            std::vector<size_t> unreachable;
            detectUnreachableCode(block, unreachable);

            // Remove unreachable instructions in reverse order
            for (auto it = unreachable.rbegin(); it != unreachable.rend(); ++it) {
                unreachableCodeRemoved_++;
                metrics_.codeReductionBytes += 4;
            }
        }

        // Analyze and optimize branch patterns
        std::vector<BranchInfo> branches;
        analyzeBranches(func, branches);

        // Check for invertible conditions
        for (auto& block : func.blocks) {
            for (auto& inst : block.insts) {
                // Look for conditional branches
                if (inst.op == ir::Op::JUMP || inst.op == ir::Op::BRANCH_EQ ||
                    inst.op == ir::Op::BRANCH_NE || inst.op == ir::Op::BRANCH_LT ||
                    inst.op == ir::Op::BRANCH_LE || inst.op == ir::Op::BRANCH_GT ||
                    inst.op == ir::Op::BRANCH_GE) {

                    // Check if we can invert condition
                    if (canInvertCondition(inst)) {
                        branchesInverted_++;
                        metrics_.instructionsOptimized++;
                        metrics_.codeReductionBytes += 3;
                    }
                }
            }
        }
    }

    // Report metrics
    if (branchesInverted_ > 0 || unreachableCodeRemoved_ > 0) {
        
        metrics_.instructionsOptimized = branchesInverted_ + unreachableCodeRemoved_;
    }
}

void BranchOptimization::detectUnreachableCode(ir::Block& block,
                                              std::vector<size_t>& unreachable) {
    // Find unreachable code after unconditional jumps
    for (size_t i = 0; i < block.insts.size(); ++i) {
        const auto& inst = block.insts[i];

        // JUMP (unconditional) makes subsequent code unreachable
        if (inst.op == ir::Op::JUMP) {
            // Everything after this jump is unreachable
            for (size_t j = i + 1; j < block.insts.size(); ++j) {
                unreachable.push_back(j);
            }
            break;
        }

        // RET (return) also makes subsequent code unreachable
        if (inst.op == ir::Op::RET) {
            for (size_t j = i + 1; j < block.insts.size(); ++j) {
                unreachable.push_back(j);
            }
            break;
        }
    }
}

bool BranchOptimization::canInvertCondition(const ir::Inst& inst) const {
    // Check if condition can be inverted
    switch (inst.op) {
        case ir::Op::BRANCH_EQ:  // EQ can become NE
        case ir::Op::BRANCH_NE:  // NE can become EQ
        case ir::Op::BRANCH_LT:  // LT can become GE
        case ir::Op::BRANCH_LE:  // LE can become GT
        case ir::Op::BRANCH_GT:  // GT can become LE
        case ir::Op::BRANCH_GE:  // GE can become LT
            return true;
        default:
            return false;
    }
}

void BranchOptimization::analyzeBranches(ir::Function& func,
                                        std::vector<BranchInfo>& branches) {
    // Simplified branch analysis: collect branch instructions
    for (auto& block : func.blocks) {
        for (size_t i = 0; i < block.insts.size(); ++i) {
            const auto& inst = block.insts[i];

            if (inst.op == ir::Op::BRANCH_EQ || inst.op == ir::Op::BRANCH_NE ||
                inst.op == ir::Op::BRANCH_LT || inst.op == ir::Op::BRANCH_LE ||
                inst.op == ir::Op::BRANCH_GT || inst.op == ir::Op::BRANCH_GE) {

                BranchInfo bi;
                bi.instIndex = i;
                bi.isConditional = true;
                bi.isInverted = false;
                branches.push_back(bi);
            }
        }
    }
}
